#include <Arduino.h>
#include <ESP8266WiFi.h>
#include "YandexUpdater.h"

const char WIFI_SSID[] PROGMEM = "***ssid***";
const char WIFI_PSWD[] PROGMEM = "***pswd***";

const char OAUTH[] PROGMEM = "***token***";

YandexUpdater yu(FPSTR(OAUTH));

void setup() {
  Serial.begin(115200);
  Serial.println();

  WiFi.persistent(false);
  WiFi.mode(WIFI_STA);
  WiFi.begin(FPSTR(WIFI_SSID), FPSTR(WIFI_PSWD));
  Serial.print(F("Connecting to WiFi"));
  while (! WiFi.isConnected()) {
    Serial.print('.');
    delay(500);
  }
  Serial.print(F(" OK (IP "));
  Serial.print(WiFi.localIP());
  Serial.println(')');

  if (yu.isUpdateAvail()) {
    Serial.print(F("Trying to update firmware..."));
    Serial.flush();
    yu.update(true);
    Serial.println(F(" FAIL!"));
  } else {
    Serial.println(F("No new firmware found"));
  }

  Serial.flush();
  ESP.deepSleep(0);
}

void loop() {}
