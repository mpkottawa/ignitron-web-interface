#include "WebUI.h"

#include <WiFi.h>
#include <LittleFS.h>
#include <ArduinoJson.h>
#include <ESPAsyncWebServer.h>
#include <AsyncTCP.h>

// ----------------- static state -----------------
static AsyncWebServer server(80);

// Handlers set by the app
static WebUI::ButtonHandler s_onButton;
static WebUI::PresetHandler s_onPreset;
static WebUI::ParamHandler  s_onParam;

// UI state that /state returns
static String s_line0, s_line1;
static int    s_cursor   = 0;
static bool   s_bt       = false;
static int    s_battery  = -1;
static int    s_bank     = -1;
static int    s_preset   = -1;

// ----------------- helpers -----------------
static void sendJsonState(AsyncWebServerRequest* request) {
  StaticJsonDocument<256> doc;
  doc["line0"]  = s_line0;
  doc["line1"]  = s_line1;
  doc["cursor"] = s_cursor;
  doc["bt"]     = s_bt ? 1 : 0;
  doc["bat"]    = s_battery;
  if (s_bank   >= 0) doc["bank"]   = s_bank;
  if (s_preset >= 0) doc["preset"] = s_preset;

  String out;
  serializeJson(doc, out);
  request->send(200, "application/json", out);
}

void WebUI::buildRoutes() {
  // Serve /data as root, with index.html as default
  server.serveStatic("/", LittleFS, "/").setDefaultFile("index.html");

  // REST: /state → JSON
  server.on("/state", HTTP_GET, [](AsyncWebServerRequest* req) {
    sendJsonState(req);
  });

  // REST: /btn?num=#
  server.on("/btn", HTTP_GET, [](AsyncWebServerRequest* req) {
    if (req->hasParam("num")) {
      int num = req->getParam("num")->value().toInt();
      if (s_onButton) s_onButton(num);
      req->send(200, "text/plain", "OK");
    } else {
      req->send(400, "text/plain", "Missing ?num");
    }
  });

  // REST: /preset?bank=X&slot=Y
  server.on("/preset", HTTP_GET, [](AsyncWebServerRequest* req) {
    if (req->hasParam("bank") && req->hasParam("slot")) {
      int bank = req->getParam("bank")->value().toInt();
      int slot = req->getParam("slot")->value().toInt();
      if (s_onPreset) s_onPreset(bank, slot);
      req->send(200, "text/plain", "OK");
    } else {
      req->send(400, "text/plain", "Missing ?bank and/or ?slot");
    }
  });

  // REST: /param?name=Gain&value=0.73
  server.on("/param", HTTP_GET, [](AsyncWebServerRequest* req) {
    if (req->hasParam("name") && req->hasParam("value")) {
      String name  = req->getParam("name")->value();
      float  value = req->getParam("value")->value().toFloat();
      if (s_onParam) s_onParam(name, value);
      req->send(200, "text/plain", "OK");
    } else {
      req->send(400, "text/plain", "Missing ?name and/or ?value");
    }
  });

  // Fallback
  server.onNotFound([](AsyncWebServerRequest* req) {
    req->send(404, "text/plain", "Not found");
  });
}

void WebUI::begin() {
  if (!LittleFS.begin(true)) {
    Serial.println("[WebUI] LittleFS mount FAILED");
  } else {
    Serial.println("[WebUI] LittleFS mounted");
  }

  // Simple AP for now so it "just works"
  WiFi.mode(WIFI_AP);
  const char* ssid = "Ignitron-AP";
  const char* pass = "ignitron123";
  if (WiFi.softAP(ssid, pass)) {
    Serial.printf("[WebUI] AP started: SSID=%s  PASS=%s  IP=%s\n",
                  ssid, pass, WiFi.softAPIP().toString().c_str());
  } else {
    Serial.println("[WebUI] softAP failed!");
  }

  buildRoutes();
  server.begin();
  Serial.println("[WebUI] HTTP server started");
}

void WebUI::loop() {
  // nothing needed for AsyncWebServer
}

void WebUI::setHandlers(ButtonHandler onButton,
                        PresetHandler onPreset,
                        ParamHandler  onParam) {
  s_onButton = onButton;
  s_onPreset = onPreset;
  s_onParam  = onParam;
}

void WebUI::pushDisplay(const char* line0,
                        const char* line1,
                        int cursor,
                        const WebUI::Icons& ic) {
  s_line0   = line0 ? line0 : "";
  s_line1   = line1 ? line1 : "";
  s_cursor  = cursor;
  s_bt      = ic.bluetooth;
  s_battery = ic.battery;
}

void WebUI::pushBankPreset(int bank, int preset) {
  s_bank   = bank;
  s_preset = preset;
}
