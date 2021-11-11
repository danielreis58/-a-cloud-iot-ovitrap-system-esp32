
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <Wire.h>
#include "Adafruit_VL53L0X.h"

// address we will assign if dual sensor is present
#define LOX1_ADDRESS 0x30
#define LOX2_ADDRESS 0x31
#define LOX3_ADDRESS 0x32
#define LOX4_ADDRESS 0x33
#define LOX5_ADDRESS 0x34
int sensor1, sensor2, sensor3, sensor4, sensor5;

// set the pins to shutdown
#define SHT_LOX1 D4
#define SHT_LOX2 D5
#define SHT_LOX3 D6
#define SHT_LOX4 D7
#define SHT_LOX5 D3

//Sensor
Adafruit_VL53L0X lox1 = Adafruit_VL53L0X();
Adafruit_VL53L0X lox2 = Adafruit_VL53L0X();
Adafruit_VL53L0X lox3 = Adafruit_VL53L0X();
Adafruit_VL53L0X lox4 = Adafruit_VL53L0X();
Adafruit_VL53L0X lox5 = Adafruit_VL53L0X();

// this holds the measurement
VL53L0X_RangingMeasurementData_t measure1;
VL53L0X_RangingMeasurementData_t measure2;
VL53L0X_RangingMeasurementData_t measure3;
VL53L0X_RangingMeasurementData_t measure4;
VL53L0X_RangingMeasurementData_t measure5;

//Cliente
HTTPClient http;
WiFiClient client;

//Variaveis
// String ssid = "VIVOFIBRA-D6586";
// String password = "rafa1D606";
String ssid = "TREME";
String password = "tanoquadro";
String id = "3e8aD6eD67-d4c8-4323-af8f-1d116D6880671";
int range = 100;

String url = "http://18.22D6.155.62:8001/catch/";

void setID()
{
  // all reset
  digitalWrite(SHT_LOX1, LOW);
  digitalWrite(SHT_LOX2, LOW);
  digitalWrite(SHT_LOX3, LOW);
  digitalWrite(SHT_LOX4, LOW);
  digitalWrite(SHT_LOX5, LOW);
  delay(10);

  // // all unreset
  // digitalWrite(SHT_LOX1, HIGH);
  // digitalWrite(SHT_LOX2, HIGH);
  // digitalWrite(SHT_LOX3, HIGH);
  // digitalWrite(SHT_LOX4, HIGH);
  // digitalWrite(SHT_LOX5, HIGH);
  // delay(10);

  // LOX1
  digitalWrite(SHT_LOX1, HIGH);
  delay(10);
  if (!lox1.begin(LOX1_ADDRESS))
  {
    Serial.println(F("Failed to boot LOX1"));
    while (1)
      ;
  }
  delay(10);

  // LOX2
  digitalWrite(SHT_LOX2, HIGH);
  delay(10);
  if (!lox2.begin(LOX2_ADDRESS))
  {
    Serial.println(F("Failed to boot LOX2"));
    while (1)
      ;
  }
  delay(10);

  // LOX3
  digitalWrite(SHT_LOX3, HIGH);
  delay(10);
  if (!lox3.begin(LOX3_ADDRESS))
  {
    Serial.println(F("Failed to boot LOX3"));
    while (1)
      ;
  }
  delay(10);

  // LOX4
  digitalWrite(SHT_LOX4, HIGH);
  delay(10);
  if (!lox4.begin(LOX4_ADDRESS))
  {
    Serial.println(F("Failed to boot LOX4"));
    while (1)
      ;
  }
  delay(10);

  // LOX5
  digitalWrite(SHT_LOX5, HIGH);
  delay(10);
  if (!lox5.begin(LOX5_ADDRESS))
  {
    Serial.println(F("Failed to boot LOX5"));
    while (1)
      ;
  }
  delay(10);
}

void readSensors()
{

  lox1.rangingTest(&measure1, false);
  lox2.rangingTest(&measure2, false);
  lox3.rangingTest(&measure3, false);
  lox4.rangingTest(&measure4, false);
  lox5.rangingTest(&measure5, false);

  // 1
  Serial.print("1: ");
  sensor1 = measure1.RangeMilliMeter;
  Serial.print(sensor1);
  Serial.print("mm ");

  // 2
  Serial.print("2: ");
  sensor2 = measure2.RangeMilliMeter;
  Serial.print(sensor2);
  Serial.print("mm ");

  // 3
  Serial.print("3: ");
  sensor3 = measure3.RangeMilliMeter;
  Serial.print(sensor3);
  Serial.print("mm ");

  // 4
  Serial.print("4: ");
  sensor4 = measure4.RangeMilliMeter;
  Serial.print(sensor4);
  Serial.print("mm ");

  // 5
  Serial.print("5: ");
  sensor5 = measure5.RangeMilliMeter;
  Serial.print(sensor5);
  Serial.println("mm");

}

void connectToWifi(String ssid, String password)
{
  WiFi.begin(ssid, password);

  Serial.print("Conectando a rede: ");
  Serial.println(ssid);

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  Serial.println("Conectado!");

  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}

void callAPI()
{
  Serial.print("Calling API: ");
  Serial.println(String(url) + String(id));

  http.begin(client, String(url) + String(id));
  http.addHeader("Content-Type", "application/json");
  String httpRequestData = "{\"number\":1}";
  int httpResponseCode = http.POST(httpRequestData);
  if (httpResponseCode > 0)
  {
    Serial.print("HTTP Response code: ");
    Serial.println(httpResponseCode);
  }
  http.end();
}

void setup()
{
  Serial.begin(9600);
  connectToWifi(ssid, password);

  pinMode(SHT_LOX1, OUTPUT);
  pinMode(SHT_LOX2, OUTPUT);
  pinMode(SHT_LOX3, OUTPUT);
  pinMode(SHT_LOX4, OUTPUT);
  pinMode(SHT_LOX5, OUTPUT);


  digitalWrite(SHT_LOX1, LOW);
  digitalWrite(SHT_LOX2, LOW);
  digitalWrite(SHT_LOX3, LOW);
  digitalWrite(SHT_LOX4, LOW);
  digitalWrite(SHT_LOX5, LOW);

  setID();
}

void loop()
{
  readSensors();
  delay(100);
}
