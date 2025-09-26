#pragma once
#include <Arduino.h>
#include <functional>

class WebUI {
public:
  struct Icons {
    bool bluetooth = false;   // 1 = connected, 0 = not
    int  battery   = -1;      // -1 = unknown, else 0..100 (or enum mapping)
  };

  using ButtonHandler = std::function<void(int)>;
  using PresetHandler = std::function<void(int,int)>;
  using ParamHandler  = std::function<void(const String&, float)>;

  static void begin();  // sets up WiFi AP + HTTP routes, serves /data/index.html
  static void loop();   // currently no-op; keep for symmetry

  static void setHandlers(ButtonHandler onButton,
                          PresetHandler onPreset,
                          ParamHandler  onParam);

  // Mirror device → browser
  static void pushDisplay(const char* line0,
                          const char* line1,
                          int cursor,
                          const WebUI::Icons& ic);

  static void pushBankPreset(int bank, int preset);

private:
  static void buildRoutes();
};
