<img width="1024" height="1024" alt="IPT" src="https://github.com/user-attachments/assets/232fbc66-9883-433b-a46b-68cead18deec" />

# Ignitron Preset Tools

The **Ignitron Preset Tools.exe** can be placed in the root directory of your `ignitron` folder:

```
/ignitron
```

To enable the Ignitron pedal to pull current pedal presets, as well as stream presets from the app, two firmware files must be modified:

- `ignitron.ino` (main Ignitron folder) → **3 edits**  
- `SparkPresetControl.cpp` (in `/src` folder) → **1 edit**

You can either manually edit these files as described below, or replace them entirely with provided versions and update pedal-specific bits (pins, LEDs, display, etc.).

---

## A. Preset Pulling Setup  
(edit `/ignitron/ignitron.ino`)

### 1. Add this include at the top with the other libraries:
```cpp
#include <LittleFS.h>
```

---

### 2. Add this line inside `void loop()`:
```cpp
handleSerialCommands();   // so it will react to LISTPRESETS
```

---

### 3. Add the following block at the **end of the file**:
```cpp
// === BEGIN: LISTPRESETS serial support =======================================

// Case-insensitive “.json” check
static bool hasJsonExt(const char *name) {
  if (!name) return false;
  size_t len = strlen(name);
  if (len < 5) return false;
  const char *ext = name + (len - 5);
  return ext[0] == '.' &&
         (ext[1] == 'j' || ext[1] == 'J') &&
         (ext[2] == 's' || ext[2] == 'S') &&
         (ext[3] == 'o' || ext[3] == 'O') &&
         (ext[4] == 'n' || ext[4] == 'N');
}

// Dump entire JSON file to a single line (removes CR/LF/TAB)
static void printJsonFileSingleLine(File &f) {
  Serial.print("JSON STRING: ");
  while (f.available()) {
    char c = (char)f.read();
    if (c == '\r' || c == '\n' || c == '\t') continue;
    Serial.write(c);
  }
  Serial.println();
}

// List every *.json at the LittleFS root
static void listAllPresets() {
  Serial.println("LISTPRESETS_START");

  File root = LittleFS.open("/");
  if (!root) {
    Serial.println("⚠️ Could not open LittleFS root");
    Serial.println("LISTPRESETS_DONE");
    return;
  }

  while (true) {
    File f = root.openNextFile();
    if (!f) break;

    if (!f.isDirectory()) {
      const char *name = f.name();
      if (name && hasJsonExt(name)) {
        Serial.print("Reading preset filename: ");
        Serial.println(name);
        printJsonFileSingleLine(f);
      }
    }
    f.close();
  }

  Serial.println("LISTPRESETS_DONE");
}

// Robust line-buffered serial command reader
static void handleSerialCommands() {
  static String buf;

  while (Serial.available()) {
    char c = (char)Serial.read();
    if (c == '\r') continue;

    if (c == '\n') {
      String cmd = buf;
      buf = "";
      cmd.trim();
      if (cmd.length() == 0) return;

      String u = cmd;
      u.toUpperCase();

      if (u == "LISTPRESETS") {
        listAllPresets();
      }
      if (u == "LISTBANKS") {
        File f = LittleFS.open("/PresetList.txt");
        if (f) {
          Serial.println("LISTBANKS_START");
          while (f.available()) {
            char c = f.read();
            if (c == '\r') continue;
            Serial.write(c);
          }
          Serial.println("LISTBANKS_DONE");
          f.close();
        } else {
          Serial.println("⚠️ PresetList.txt not found");
          Serial.println("LISTBANKS_DONE");
        }
      }
    } else {
      buf += c;
      if (buf.length() > 256) {
        buf.remove(0, buf.length() - 256);
      }
    }
  }
}

// === END: LISTPRESETS serial support =========================================
```

---

## B. Spark App Streaming Setup  
(edit `/ignitron/src/SparkPresetControl.cpp`)

Add the following lines **after** `DEBUG_PRINTLN(appReceivedPreset_.json.c_str());` inside  
`SparkPresetControl::updateFromSparkResponseAmpPreset`:

```cpp
// 🔧 Added for App Scraper
Serial.println("received from app:");
Serial.println(appReceivedPreset_.json.c_str());
```

### Final snippet should look like:
```cpp
void SparkPresetControl::updateFromSparkResponseAmpPreset(char *presetJson) {
    presetEditMode_ = PRESET_EDIT_STORE;
    appReceivedPreset_ = presetBuilder.getPresetFromJson(presetJson);

    DEBUG_PRINTLN("received from app:");
    DEBUG_PRINTLN(appReceivedPreset_.json.c_str());

    // 🔧 Added for App Scraper
    Serial.println("received from app:");
    Serial.println(appReceivedPreset_.json.c_str());

    presetNumToEdit_ = 0;
}
```

---

## Building
After edits, **compile and flash to pedal**.  
Enjoy!

---

# Ignitron Preset Tools Operation

## Overview
- Ensure your pedal connects to your device running the Spark app (BLE or SRL).  
- Preset pulling only works when the pedal is connected via USB in **AMP mode** (hold switch 1 while booting).  
- Keep switch 1 pressed while the tool starts its connection. The pedal will reboot, and you can release switch 1 after the screen cycles.  

---

## Operation

### Main Menu
When you run `ignitron_preset_tools.exe`, you’ll see 4 options:

```
1. Preset Picker
2. Preset Puller
3. App Scraper
4. Exit
```

---

### 1. Preset Picker
**Description:**  
Load your `/data` folder of presets, organize them in the GUI, and export the 2 files needed to build your preset banks.
