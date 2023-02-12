#include <Arduino.h>
#include <ThingerESP32.h>
#include <Wire.h>
#include "Adafruit_SHT31.h"
#include <SPI.h>
#include "Adafruit_LTR329_LTR303.h"
#include <Pushsafer.h>
#include <ArduinoJson.h>
#include <WiFiClient.h>
#include <Pushsafer.h>

#define USERNAME ""
#define DEVICE_ID ""
#define DEVICE_CREDENTIAL ""
#define SSID ""
#define SSID_PASSWORD ""
#define PushsaferKey "" // Pushsafer private or alias key

char ssid[] = "";     // your network SSID (name)
char password[] = ""; // your network key
bool daylight = true;

/*WiFiClientSecure client;*/
WiFiClient client;
Pushsafer pushsafer(PushsaferKey, client);
Adafruit_SHT31 sht31 = Adafruit_SHT31();
Adafruit_LTR329 ltr = Adafruit_LTR329();
uint8_t loopCnt = 0;
uint16_t v_p_i, inf;
ThingerESP32 thing(USERNAME, DEVICE_ID, DEVICE_CREDENTIAL);

struct PushSaferInput input_temp;
struct PushSaferInput input_hum;
struct PushSaferInput input_light;

void setup() {
  // open serial for debugging
  Serial.begin(115200);

  pinMode(16, OUTPUT);

  thing.add_wifi(SSID, SSID_PASSWORD);

  // digital pin control example (i.e. turning on/off a light, a relay, configuring a parameter, etc)
  thing["GPIO_16"] << digitalPin(16);

  // resource output example (i.e. reading a sensor value)
  thing["millis"] >> outputValue(millis());

  // more details at http://docs.thinger.io/arduino/

  Serial.println("SHT31 test");
  if (! sht31.begin(0x44)) {   // Set to 0x45 for alternate i2c addr
    Serial.println("Couldn't find SHT31 sensor!");
    while (1) delay(1);
  }
  Serial.println("Found SHT sensor!");

  // Set gain of 1 (see advanced demo for all options!
  ltr.setGain(LTR3XX_GAIN_1);
  // Set integration time of 50ms (see advanced demo for all options!
  ltr.setIntegrationTime(LTR3XX_INTEGTIME_50);
  // Set measurement rate of 50ms (see advanced demo for all options!
  ltr.setMeasurementRate(LTR3XX_MEASRATE_50);

  Serial.println("LTR test");
  if ( ! ltr.begin() ) {
    Serial.println("Couldn't find LTR sensor!");
    while (1) delay(10);
  }
  Serial.println("Found LTR sensor!");

  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);

  // Attempt to connect to Wifi network:
  Serial.print("Connecting Wifi: ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  // Take a look at the Pushsafer.com API at
  // https://www.pushsafer.com/en/pushapi

  input_temp.sound = "8";
  input_temp.vibration = "1";
  input_temp.icon = "1";
  input_temp.iconcolor = "#FFCCCC";
  input_temp.priority = "1";
  input_temp.device = "62734";
  input_temp.url = "https://www.pushsafer.com";
  input_temp.urlTitle = "Open Pushsafer.com";

  input_hum = input_temp;
  input_light = input_temp;
}

void loop() {
  float t = sht31.readTemperature();
  float h = sht31.readHumidity();

  if (! isnan(t)) {  // check if 'is not a number'
    Serial.print("Temp *C = "); Serial.print(t); Serial.print("\t\t");
  } else { 
    Serial.println("Failed to read temperature");
  }
  
  if (! isnan(h)) {  // check if 'is not a number'
    Serial.print("Hum. % = "); Serial.println(h);
  } else { 
    Serial.println("Failed to read humidity");
  }


  bool valid;
  uint16_t visible_plus_ir, infrared;

  if (ltr.newDataAvailable()) {
    valid = ltr.readBothChannels(visible_plus_ir, infrared);
    if (valid) {
      Serial.print("CH0 Visible + IR: ");
      Serial.print(visible_plus_ir);
      v_p_i = visible_plus_ir;
      Serial.print("\t\tCH1 Infrared: ");
      Serial.println(infrared);
      inf = infrared;
    }
    delay(100);
  }

  // define the resource just once in the setup() section

  thing["temphum"] >> [](pson &out){ 
    out["temperature"] = sht31.readTemperature();
    out["humidity"] = sht31.readHumidity();
    out["light"] = v_p_i;
  };

  thing.handle();
  thing.stream("temphum");

  //Temperature warnings

  if ( t >= 30.0 ){
    input_temp.message = "Temperature: " + String(t);
    input_temp.title = "zu warm";

  }else if ( t <= 25.0 ){
    input_temp.message = "Temperature: " + String(t);
    input_temp.title = "too cold!";
  }
  pushsafer.sendEvent(input_temp);
  Serial.println("Sent");

  //humidity warnings

  if ( h >= 60.0 ){
    input_hum.message = "Humidity: " + String(h);
    input_hum.title = "too moist!";
  }else if ( h <= 30.0 ){
    input_hum.message = "Humidity: " + String(h);
    input_hum.title = "too dry!";
  }
  pushsafer.sendEvent(input_hum);
  Serial.println("Sent");

  // light warnings

  if (v_p_i <= 10.0){
    if (daylight) {
      daylight = false;
      input_light.message = "light= " + String(v_p_i);
      input_light.title = "dark";
      pushsafer.sendEvent(input_light);
      Serial.println("Sent");
    }
  } else {
    if (!daylight){
      daylight= true;
      input_light.message = "light= " + String(v_p_i);
      input_light.title = "bright";
      pushsafer.sendEvent(input_light);
      Serial.println("Sent");
    }
  }

 delay(6000);
}