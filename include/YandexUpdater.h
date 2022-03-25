#pragma once

#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <ESP8266HTTPClient.h>
#include <Updater.h>
#include <ArduinoJson.h>

class YandexUpdater {
public:
  YandexUpdater(const String &token);
  ~YandexUpdater();

  bool isUpdateAvail();
  bool update(bool reboot);

protected:
  void prepare();
  String getFwDownloadUrl();

  static const char FW_NAME[] PROGMEM;

  String _token;
  WiFiClientSecure _client;
  HTTPClient *_http;
};

YandexUpdater::YandexUpdater(const String &token) : _token(token), _http(nullptr) {
  _client.setInsecure();
}

YandexUpdater::~YandexUpdater() {
  if (_http)
    delete _http;
}

bool YandexUpdater::isUpdateAvail() {
  static const char URL[] PROGMEM = "https://cloud-api.yandex.net/v1/disk/resources?path=";
  static const char FIELDS[] PROGMEM = "&fields=md5";

  bool result = false;

  _http = new HTTPClient();
  if (_http) {
    String url;

    url = FPSTR(URL);
    url.concat(FPSTR(FW_NAME));
    url.concat(FPSTR(FIELDS));
    if (_http->begin(_client, url)) {
      url.clear();
      prepare();
      if (_http->GET() == 200) {
        DynamicJsonDocument json(1024);

        if (deserializeJson(json, _http->getStream()) == DeserializationError::Ok) {
          url = (const char*)json[F("md5")];
          result = ! url.equalsIgnoreCase(ESP.getSketchMD5());
        }
      }
      _http->end();
    }
    delete _http;
    _http = nullptr;
  }
  return result;
}

bool YandexUpdater::update(bool reboot) {
  String url;
  bool result = false;

  url = getFwDownloadUrl();
  if (! url.isEmpty()) {
    _http = new HTTPClient();
    if (_http) {
      if (_http->begin(_client, url)) {
        url.clear();
        prepare();
        if (_http->GET() == 200) {
          int size = _http->getSize();

          if (size > 0) {
            if (size <= (int)ESP.getFreeSketchSpace()) {
              uint8_t header[4];
              WiFiClient *tcp = _http->getStreamPtr();

              if (tcp->peekBytes(header, sizeof(header)) == sizeof(header)) {
                if ((header[0] == 0xE9) && (ESP.magicFlashChipSize((header[3] & 0xF0) >> 4) <= ESP.getFlashChipRealSize())) {
                  WiFiUDP::stopAll();
                  WiFiClient::stopAllExcept(tcp);
                  if (Update.begin(size, U_FLASH)) {
                    if (Update.writeStream(*tcp) == (size_t)size) {
                      if (Update.end()) {
                        result = true;
                      }
                    }
                  }
                }
              }
            }
          }
        }
        _http->end();
      }
      delete _http;
      _http = nullptr;
    }
    if (result && reboot) {
      ESP.restart();
    }
  }
  return result;
}

void YandexUpdater::prepare() {
  _http->setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
  _http->setTimeout(15000);
  _http->addHeader(F("Authorization"), String(F("OAuth ")) + _token);
}

String YandexUpdater::getFwDownloadUrl() {
  static const char URL[] PROGMEM = "https://cloud-api.yandex.net/v1/disk/resources/download?path=";
  static const char FIELDS[] PROGMEM = "&fields=href";

  String result;

  _http = new HTTPClient();
  if (_http) {
    result = FPSTR(URL);
    result.concat(FPSTR(FW_NAME));
    result.concat(FPSTR(FIELDS));
    if (_http->begin(_client, result)) {
      result.clear();
      prepare();
      if (_http->GET() == 200) {
        DynamicJsonDocument json(1024);

        if (deserializeJson(json, _http->getStream()) == DeserializationError::Ok) {
          result = (const char*)json[F("href")];
        }
      }
      _http->end();
    }
    delete _http;
    _http = nullptr;
  }
  return result;
}

const char YandexUpdater::FW_NAME[] PROGMEM = "app%3A%2Ffirmware.bin";
