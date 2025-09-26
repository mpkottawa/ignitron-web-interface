#pragma once
#include <Arduino.h>
#include <functional>

class WebUI {
public:
    struct Icons {
        bool bluetooth = false;
        int  battery   = -1;   // -1 = unknown / not available
    };

    // Start HTTP + WebSocket server (serve /index.html out of LittleFS and /ws socket)
    static void begin();

    // For AsyncWebServer nothing is needed here, but we keep it for symmetry
    static void loop();

    // Push a display update to all connected browser clients
    static void pushDisplay(const char* line0,
                            const char* line1,
                            int cursor,
                            const Icons& ic,
                            int bank   = -1,
                            int preset = -1);

    // Handlers the firmware can supply so browser actions control the pedal
    using ButtonHandler    = std::function<void(int)>;
    using PresetHandler    = std::function<void(int,int)>;
    using ParameterHandler = std::function<void(const String&, float)>;

    static void setHandlers(ButtonHandler btn,
                            PresetHandler  preset,
                            ParameterHandler param);
};
