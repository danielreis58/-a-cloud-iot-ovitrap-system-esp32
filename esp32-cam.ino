#include <Wire.h>
#include <VL53L0X.h>

#include "esp_camera.h"
#include "esp_wifi.h"
#include <HTTPClient.h>
#include <StreamString.h>
// Time
#include "time.h"

// MicroSD
#include "FS.h"
#include "SD.h"
#include "SPI.h"

//Rede Wifi
const char* ssidWifi = "Treme 2.4GHz";
const char* passwordWifi = "tremeroberval";

//Rede Wifi GoPro
const char* ssidGoPro = "goprohero3";
const char* passwordGoPro = "goprohero";

int capture_interval = 180000; // microseconds between captures
int upload_interval = 300000; // microseconds between uploads

long current_millis;
long last_upload_millis = 0;
long last_capture_millis = 0;
static esp_err_t cam_err;
static esp_err_t card_err;
char strftime_buf[64];
int file_number = 0;
bool internet_connected = false;
bool sdcard_mounted = false;
bool sensor_connected = false;

// CAMERA_MODEL_AI_THINKER
#define PWDN_GPIO_NUM     32
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM      0
#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27
#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM        5
#define VSYNC_GPIO_NUM    25
#define HREF_GPIO_NUM     23
#define PCLK_GPIO_NUM     22


#define SERVER     "192.168.0.181"
#define PORT     8001
#define BOUNDARY     "--------------------------133747188241686651551404"
#define TIMEOUT      20000


VL53L0X sensor;
WiFiClient client;

void setup() {
  Serial.begin(115200);

  if (init_wifi(ssidWifi, passwordWifi)) { // Connect to WiFi
    internet_connected = true;
    Serial.println("Internet Connected");
  }
//  if (init_sdcard()) { // Mount SD Card
//    sdcard_mounted = true;
//    Serial.println("SD Card Mounted");
//  }

  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;
  //init with high specs to pre-allocate larger buffers
  if (psramFound()) {
    config.frame_size = FRAMESIZE_UXGA;
    config.jpeg_quality = 10;
    config.fb_count = 2;
  } else {
    config.frame_size = FRAMESIZE_SVGA;
    config.jpeg_quality = 12;
    config.fb_count = 1;
  }

  //camera init
  cam_err = esp_camera_init(&config);
  if (cam_err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", cam_err);
    return;
  }

  deleteFiles(SD, "/CAMERA");
  deleteFiles(SD, "/GOPRO");

  createDir(SD, "/CAMERA");
  createDir(SD, "/GOPRO");

  if (init_sensor()) { // Mount SD Card
    sensor_connected = true;
    Serial.println("Sensor Connected");
  }
}

void listDir(fs::FS &fs, const char * dirname, uint8_t levels) {
  Serial.printf("Listing directory: %s\n", dirname);

  File root = fs.open(dirname);
  if (!root) {
    Serial.println("Failed to open directory");
    return;
  }
  if (!root.isDirectory()) {
    Serial.println("Not a directory");
    return;
  }

  File file = root.openNextFile();
  while (file) {
    if (file.isDirectory()) {
      Serial.print("  DIR : ");
      Serial.println(file.name());
      if (levels) {
        listDir(fs, file.name(), levels - 1);
      }
    } else {
      Serial.print("  FILE: ");
      Serial.print(file.name());
      Serial.print("  SIZE: ");
      Serial.println(file.size());
    }
    file = root.openNextFile();
  }
}

void createDir(fs::FS &fs, const char * path) {
  Serial.printf("Creating Dir: %s\n", path);
  if (fs.mkdir(path)) {
    Serial.println("Dir created");
  } else {
    Serial.println("mkdir failed");
  }
}

void removeDir(fs::FS &fs, const char * path) {
  Serial.printf("Removing Dir: %s\n", path);
  if (fs.rmdir(path)) {
    Serial.println("Dir removed");
  } else {
    Serial.println("rmdir failed");
  }
}

void readFile(fs::FS &fs, const char * path) {
  Serial.printf("Reading file: %s\n", path);

  File file = fs.open(path);
  if (!file) {
    Serial.println("Failed to open file for reading");
    return;
  }

  Serial.print("Read from file: ");
  while (file.available()) {
    Serial.write(file.read());
  }
  file.close();
}

void writeFile(fs::FS &fs, const char * path, const char * message) {
  Serial.printf("Writing file: %s\n", path);

  File file = fs.open(path, FILE_WRITE);
  if (!file) {
    Serial.println("Failed to open file for writing");
    return;
  }
  if (file.print(message)) {
    Serial.println("File written");
  } else {
    Serial.println("Write failed");
  }
  file.close();
}

void appendFile(fs::FS &fs, const char * path, const char * message) {
  Serial.printf("Appending to file: %s\n", path);

  File file = fs.open(path, FILE_APPEND);
  if (!file) {
    Serial.println("Failed to open file for appending");
    return;
  }
  if (file.print(message)) {
    Serial.println("Message appended");
  } else {
    Serial.println("Append failed");
  }
  file.close();
}

void renameFile(fs::FS &fs, const char * path1, const char * path2) {
  Serial.printf("Renaming file %s to %s\n", path1, path2);
  if (fs.rename(path1, path2)) {
    Serial.println("File renamed");
  } else {
    Serial.println("Rename failed");
  }
}

void deleteFile(fs::FS &fs, const char * path) {
  Serial.printf("Deleting file: %s\n", path);
  if (fs.remove(path)) {
    Serial.println("File deleted");
  } else {
    Serial.println("Delete failed");
  }
}

void deleteFiles(fs::FS &fs, const char * dirname) {
  Serial.printf("Deleting files on directory: %s\n", dirname);

  File root = fs.open(dirname);
  if (!root) {
    Serial.println("Failed to open directory");
    return;
  }
  if (!root.isDirectory()) {
    Serial.println("Not a directory");
    return;
  }

  File file = root.openNextFile();
  while (file) {
    if (fs.remove(file.name())) {
      Serial.print("File deleted: ");
      Serial.println(file.name());
    } else {
      Serial.println("Delete failed");
    }
    file = root.openNextFile();
  }
}

void testFileIO(fs::FS &fs, const char * path) {
  File file = fs.open(path);
  static uint8_t buf[512];
  size_t len = 0;
  uint32_t start = millis();
  uint32_t end = start;
  if (file) {
    len = file.size();
    size_t flen = len;
    start = millis();
    while (len) {
      size_t toRead = len;
      if (toRead > 512) {
        toRead = 512;
      }
      file.read(buf, toRead);
      len -= toRead;
    }
    end = millis() - start;
    Serial.printf("%u bytes read for %u ms\n", flen, end);
    file.close();
  } else {
    Serial.println("Failed to open file for reading");
  }


  file = fs.open(path, FILE_WRITE);
  if (!file) {
    Serial.println("Failed to open file for writing");
    return;
  }

  size_t i;
  start = millis();
  for (i = 0; i < 2048; i++) {
    file.write(buf, 512);
  }
  end = millis() - start;
  Serial.printf("%u bytes written for %u ms\n", 2048 * 512, end);
  file.close();
}

String setFileName() {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    Serial.println("Failed to obtain time");
    return "Fail";
  }
  char timeStringBuff[50]; //50 chars should be enough
  strftime(timeStringBuff, sizeof(timeStringBuff), "%B%d%Y%H%M%S", &timeinfo);
  String localTime(timeStringBuff);
  return localTime;
}

bool init_wifi(const char * ssid, const char * password) {
  int connAttempts = 0;
  Serial.println("\r\nConnecting to: " + String(ssid));
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED ) {
    delay(500);
    Serial.print(".");
    if (connAttempts > 30) return false;
    connAttempts++;
  }
  Serial.println(WiFi.localIP());
  configTime(0, -10800, "pool.ntp.org");
  return true;
}

bool init_sdcard() {
  SPI.begin(14, 2, 15, 13);
  if (!SD.begin(13)) {
    Serial.println("Card Mount Failed");
    return false;
  }
  uint8_t cardType = SD.cardType();

  if (cardType == CARD_NONE) {
    Serial.println("No SD card attached");
    return false;
  }

  Serial.print("SD Card Type: ");
  if (cardType == CARD_MMC) {
    Serial.println("MMC");
  } else if (cardType == CARD_SD) {
    Serial.println("SDSC");
  } else if (cardType == CARD_SDHC) {
    Serial.println("SDHC");
  } else {
    Serial.println("UNKNOWN");
  }

  uint64_t cardSize = SD.cardSize() / (1024 * 1024);
  Serial.printf("SD Card Size: %lluMB\n", cardSize);

  Serial.printf("Total space: %lluMB\n", SD.totalBytes() / (1024 * 1024));
  Serial.printf("Used space: %lluMB\n", SD.usedBytes() / (1024 * 1024));
  return true;
}

bool init_sensor(){
  Wire.begin(3, 1);

  sensor.setTimeout(500);
  if (!sensor.init())
  {
    Serial.println("Failed to detect and initialize sensor!");
    while (1) {}
  }  
  // increase laser pulse periods (defaults are 14 and 10 PCLKs)
  sensor.setSignalRateLimit(0.1);
  sensor.setVcselPulsePeriod(VL53L0X::VcselPeriodPreRange, 18);
  sensor.setVcselPulsePeriod(VL53L0X::VcselPeriodFinalRange, 14);
  sensor.setMeasurementTimingBudget(10000);
  sensor.startContinuous(0);
}

String sendMosquito(String token, String message, uint8_t *data_pic, size_t size_pic) {
  String bodyTxt =  body("name", message);
  String bodyPic =  body("imageFile", message);
  String bodyEnd =  String("--") + BOUNDARY + String("--\r\n");
  size_t allLen = bodyTxt.length() + bodyPic.length() + size_pic + bodyEnd.length();
  String headerTxt =  header(token, "/mosquitos/files/upload", allLen);

  if (!client.connect(SERVER, PORT))
  {
    return ("connection failed");
  }

  client.print(headerTxt + bodyTxt + bodyPic);
  client.write(data_pic, size_pic);
  client.print("\r\n" + bodyEnd);

  delay(20);
  long tOut = millis() + TIMEOUT;
  while (client.connected() && tOut > millis())
  {
    if (client.available())
    {
      String serverRes = client.readStringUntil('\r');
      return (serverRes);
    }
  }
}

String sendLarva(String token, String message, File file) {
  String bodyTxt =  body("name", message);
  String bodyPic =  body("imageFile", message);
  String bodyEnd =  String("--") + BOUNDARY + String("--\r\n");
  size_t allLen = bodyTxt.length() + bodyPic.length() + file.size() + bodyEnd.length();
  String headerTxt =  header(token, "/larvas/files/upload", allLen);

  if (!client.connect(SERVER, PORT))
  {
    return ("connection failed");
  }

  client.print(headerTxt + bodyTxt + bodyPic);
  client.write(file);
  client.print("\r\n" + bodyEnd);

  delay(20);
  long tOut = millis() + TIMEOUT;
  while (client.connected() && tOut > millis())
  {
    if (client.available())
    {
      String serverRes = client.readStringUntil('\r');
      return (serverRes);
    }
  }

}

String header(String token, String url, size_t length) {
  String  data;
  data =  F("POST ");
  data += url;
  data += F(" HTTP/1.1\r\n");
  data += F("cache-control: no-cache\r\n");
  data += F("Content-Type: multipart/form-data; boundary=");
  data += BOUNDARY;
  data += "\r\n";
  data += F("User-Agent: PostmanRuntime/6.4.1\r\n");
  data += F("Accept: */*\r\n");
  data += F("Host: ");
  data += SERVER;
  data += F("\r\n");
  data += F("accept-encoding: gzip, deflate\r\n");
  data += F("Connection: keep-alive\r\n");
  data += F("content-length: ");
  data += String(length);
  data += "\r\n";
  data += "\r\n";
  return (data);
}

String body(String content , String message) {
  String data;
  data = "--";
  data += BOUNDARY;
  data += F("\r\n");
  if (content == "imageFile")
  {
    data += F("Content-Disposition: form-data; name=\"file\"; filename=\"");
    data += message;
    data += F("\"\r\n");
    data += F("Content-Type: image/jpeg\r\n");
    data += F("\r\n");
  }
  else
  {
    data += "Content-Disposition: form-data; name=\"" + content + "\"\r\n";
    data += "\r\n";
    data += message;
    data += "\r\n";
  }
  return (data);
}

void save_photo() {
  file_number++;
  String filename = setFileName() + ".jpg";
  Serial.print("Taking picture: ");
  Serial.println(file_number);

  camera_fb_t*fb = NULL;

  // Take Picture with Camera
  fb = esp_camera_fb_get();
  delay(1000);
  if (!fb) {
    Serial.println("Camera capture failed");
    return;
  }

  Serial.println("Sending Mosquito");
  String res = sendMosquito("token", filename, fb->buf, fb->len);
  Serial.println(res);

  File file = SD.open(("/CAMERA/" + filename).c_str(), "w");
  if (file != NULL)  {
    file.write(fb->buf, fb->len);
    Serial.printf("File saved: %s\n", ("/CAMERA/" + filename).c_str());
  }  else  {
    Serial.println("Could not save file");
  }
  esp_camera_fb_return(fb);
  file.close();
}

void sendAllSDPhotos() {
  File root = SD.open("/GOPRO");
  File file = root.openNextFile();
  while (file) {
    String filename = String(file.name()).substring(7);
    Serial.println(filename);

    Serial.println("Sending Larva");
    String res = sendLarva("token", filename , file);
    Serial.println(res);

    if (SD.remove(file.name())) {
      Serial.print("File deleted: ");
      Serial.println(file.name());
    } else {
      Serial.println("Delete failed");
    }
    file = root.openNextFile();
  }
}

void photo_mode() {
  HTTPClient http;
  http.begin("http://10.5.5.9/camera/CM?t=goprohero&p=%01");
  Serial.println("Photo Mode");
  Serial.println(http.GET());
  delay(1000);
  http.end();
}

void shutter() {
  HTTPClient http;
  http.begin("http://10.5.5.9/bacpac/SH?t=goprohero&p=%01");
  Serial.println("Shutter");
  Serial.println(http.GET());
  delay(1000);
  http.end();
}

String get_filename() {
  HTTPClient http;
  http.begin("10.5.5.9", 8080, "/DCIM/100GOPRO/");
  Serial.println("Get DCIM");
  Serial.println(http.GET());
  String filename = http.getString().substring(826, 838);
  Serial.println(filename);
  delay(1000);
  http.end();
  return filename;
}

void save_DCIM(String fileaddress) {
  String filename = setFileName() + ".jpg";
  File file = SD.open(("/GOPRO/" + filename).c_str(), FILE_WRITE);
  HTTPClient http;
  http.begin("10.5.5.9", 8080, "http://10.5.5.9:8080/DCIM/100GOPRO/" + fileaddress);
  Serial.println("Get JPG");
  Serial.println(http.GET());
  http.writeToStream(&file);
  delay(1000);
  http.end();
  file.close();
}

void delete_all() {
  HTTPClient http;
  http.begin("http://10.5.5.9/camera/DA?t=goprohero");
  Serial.println("Delete All");
  Serial.println(http.GET());
  delay(1500);
  http.end();

}

void loop()
{
//  current_millis = millis();
  
  if (sensor.timeoutOccurred()) {
    //Serial.print(" TIMEOUT");
  }
  if (sensor.readRangeContinuousMillimeters() < 250) {
    //Serial.println(sensor.readRangeContinuousMillimeters());
    save_photo();
  }

  

  
//  if (current_millis - last_capture_millis > capture_interval) { // Take another picture
//    client.stop();
//    client.flush();
//    WiFi.disconnect();
//    delay(10000);
//    internet_connected = false;
//    if (init_wifi(ssidGoPro, passwordGoPro)) { // Connected to WiFi
//      internet_connected = true;
//      Serial.println("Internet Connected");
//    }
//
//    if (internet_connected) {
//      photo_mode();
//      shutter();
//      save_DCIM(get_filename());
//      delete_all();
//    }
//
//    client.stop();
//    client.flush();
//    WiFi.disconnect();
//    delay(10000);
//    internet_connected = false;
//    if (init_wifi(ssidWifi, passwordWifi)) { // Connect to WiFi
//      internet_connected = true;
//      Serial.println("Internet Connected");
//    }
//    last_capture_millis = millis();
//  }
//
//  if (current_millis - last_upload_millis > upload_interval) {
//    sendAllSDPhotos();
//    last_upload_millis = millis();
//  }
}
