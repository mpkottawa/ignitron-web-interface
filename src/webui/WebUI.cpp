#include "WebUI.h"

#include <WiFi.h>
#include <LittleFS.h>
#include <ESPAsyncWebServer.h>
#include <AsyncTCP.h>
#include <ArduinoJson.h>

// --------------------
// statics (internal)
// --------------------
namespace {
  AsyncWebServer server(80);
  AsyncWebSocket ws("/ws");

  WebUI::ButtonHandler     g_btnHandler   = nullptr;
  WebUI::PresetHandler     g_presetHandler= nullptr;
  WebUI::ParameterHandler  g_paramHandler = nullptr;

  // Send a small hello when a client connects so UI can show status
  void sendHello(AsyncWebSocketClient* client) {
    StaticJsonDocument<128> doc;
    doc["hello"] = "ignitron";
    String json; serializeJson(doc, json);
    client->text(json);
  }

  void onWsEvent(AsyncWebSocket * /*server*/,
                 AsyncWebSocketClient *client,
                 AwsEventType type,
                 void *arg, uint8_t *data, size_t len) {

    if (type == WS_EVT_CONNECT) {
      sendHello(client);
      return;
    }

    if (type != WS_EVT_DATA) return;

    AwsFrameInfo *info = reinterpret_cast<AwsFrameInfo*>(arg);
    if (!info || info->final != 1 || info->index != 0 || info->len != len || info->opcode != WS_TEXT) {
      return;
    }

    // Parse one JSON action
    StaticJsonDocument<256> doc;
    DeserializationError err = deserializeJson(doc, data, len);
    if (err) {
      // ignore malformed messages
      return;
    }

    const char* action = doc["action"] | "";
    if (strcmp(action, "button") == 0) {
      int id = doc["id"] | 0;
      if (g_btnHandler) g_btnHandler(id);
      return;
    }

    if (strcmp(action, "preset") == 0) {
      int bank = doc["bank"] | -1;
      int slot = doc["slot"] | -1;
      if (bank >= 0 && slot >= 0 && g_presetHandler) g_presetHandler(bank, slot);
      return;
    }

    if (strcmp(action, "param") == 0) {
      const char* name = doc["name"] | "";
      float value = doc["value"] | 0.0f;
      if (name[0] && g_paramHandler) g_paramHandler(String(name), value);
      return;
    }
  }
} // namespace

// --------------------
// WebUI public API
// --------------------

void WebUI::begin() {
  // Serve root (index.html) and everything under /
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    if (LittleFS.exists("/index.html")) {
      request->send(LittleFS, "/index.html", "text/html");
    } else {
      request->send(200, "text/plain", "Ignitron WebUI: /data/index.html not found. Upload LittleFS.");
    }
  });

  // Allow static assets (css/js) if you add them later
  server.serveStatic("/", LittleFS, "/");

  // WebSocket
  ws.onEvent(onWsEvent);
  server.addHandler(&ws);

  server.begin();
}

void WebUI::loop() {
  // Async server has no loop requirement; keep for potential future polling
}

void WebUI::setHandlers(ButtonHandler btn,
                        PresetHandler  preset,
                        ParameterHandler param) {
  g_btnHandler    = btn;
  g_presetHandler = preset;
  g_paramHandler  = param;
}

void WebUI::pushDisplay(const char* line0,
                        const char* line1,
                        int cursor,
                        const Icons& ic,
                        int bank,
                        int preset) {
  StaticJsonDocument<320> doc;
  doc["line0"]  = line0 ? line0 : "";
  doc["line1"]  = line1 ? line1 : "";
  doc["cursor"] = cursor;
  doc["bt"]     = ic.bluetooth;
  doc["bat"]    = ic.battery;
  if (bank   >= 0) doc["bank"]   = bank;
  if (preset >= 0) doc["preset"] = preset;

  String json;
  serializeJson(doc, json);
  ws.textAll(json);
}
