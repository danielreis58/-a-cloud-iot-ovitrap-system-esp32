
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <Wire.h>
#include <VL53L0X.h>

#define pino_trigger D1
#define pino_echo D2
#define pino_led D3

//Sensor
VL53L0X sensor;

//Cliente
HTTPClient http;
WiFiClient client;

//Variaveis
String ssid = "VIVOFIBRA-9586";
String password = "rafa1906";
// String ssid = "TREME";
// String password = "tanoquadro";
String id = "3e8a9e97-d4c8-4323-af8f-1d1169880671";
int range = 100;

String url = "http://18.229.155.62:8001/catch/";

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

void initSensor()
{
  Serial.println("Configurando sensor...");
  Wire.begin();
  sensor.setTimeout(500);
  if (!sensor.init())
  {
    Serial.println("Erro ao iniciar sensor...");
  }
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
  initSensor();
  Serial.println("Lendo dados do sensor...");
}

void loop()
{
  int dist = sensor.readRangeSingleMillimeters();

  Serial.print(dist);
  Serial.println(" mm");

  if (dist < range)
  {
    callAPI();
  }

  if (sensor.timeoutOccurred())
  {
    Serial.print(" TIMEOUT");
  }

  delay(100);
}
