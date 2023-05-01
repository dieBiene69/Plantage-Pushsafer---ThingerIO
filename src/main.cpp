#include <Arduino.h>
#include <ThingerESP32.h>
#include <Wire.h>
#include "Adafruit_SHT31.h"
#include <SPI.h>
#include "Adafruit_LTR329_LTR303.h"
#include <ArduinoJson.h>
#include <WiFiClient.h>
#include <secrets.h>
#include <HTTPClient.h>

Adafruit_SHT31 sht31 = Adafruit_SHT31();
Adafruit_LTR329 ltr = Adafruit_LTR329();

void setup()
{
  Serial.begin(115200);

  pinMode(16, OUTPUT);

  Serial.println("SHT31 test");
  if (!sht31.begin(0x44))
  { // Set to 0x45 for alternate i2c addr
    Serial.println("Couldn't find SHT31 sensor!");
    while (1)
      delay(1);
  }
  Serial.println("Found SHT sensor!");

  // Set gain of 1 (see advanced demo for all options!
  ltr.setGain(LTR3XX_GAIN_1);
  // Set integration time of 50ms (see advanced demo for all options!
  ltr.setIntegrationTime(LTR3XX_INTEGTIME_50);
  // Set measurement rate of 50ms (see advanced demo for all options!
  ltr.setMeasurementRate(LTR3XX_MEASRATE_50);

  Serial.println("LTR test");
  if (!ltr.begin())
  {
    Serial.println("Couldn't find LTR sensor!");
    while (1)
      delay(10);
  }
  Serial.println("Found LTR sensor!");

  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);

  // Attempt to connect to Wifi network:
  Serial.print("Connecting Wifi: ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.print(".");
    delay(500);
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}

void loop()
{
  float t = sht31.readTemperature();
  float h = sht31.readHumidity();

  if (!isnan(t))
  { // check if 'is not a number'
    Serial.print("Temp *C = ");
    Serial.print(t);
    Serial.print("\t\t");
  }
  else
  {
    Serial.println("Failed to read temperature");
  }

  if (!isnan(h))
  { // check if 'is not a number'
    Serial.print("Hum. % = ");
    Serial.println(h);
  }
  else
  {
    Serial.println("Failed to read humidity");
  }

  bool valid;
  uint16_t visible_plus_ir, infrared;

  if (ltr.newDataAvailable())
  {
    valid = ltr.readBothChannels(visible_plus_ir, infrared);
    if (valid)
    {
      Serial.print("CH0 Visible + IR: ");
      Serial.print(visible_plus_ir);
      Serial.print("\t\tCH1 Infrared: ");
      Serial.println(infrared);
    }
  }

//connect with API
  if (WiFi.status() == WL_CONNECTED)
  {

    StaticJsonDocument<96> doc;
    doc["device"] = DEVICE;
    doc["temp"] = t ;
    doc["hum"] = h ;
    doc["light"] = visible_plus_ir;
    String output;

    serializeJson(doc, output);
    Serial.println(output);


    HTTPClient http;

    http.begin(API_URI);
    http.addHeader("Content-Type", "application/json");
    int httpResponseCode = http.POST(output);
    if (httpResponseCode > 0)
    {
      String response = http.getString();
      Serial.println(httpResponseCode);
      Serial.println(response);
    }
    else
    {
      Serial.print("Error on sending POST: ");
      Serial.println(httpResponseCode);
    }
    http.end();
  }
  else

  {
    Serial.println("Error in WiFi connection");
  }
  delay(5000);
}
