
<img width="1024" height="1024" alt="IPT" src="https://github.com/user-attachments/assets/232fbc66-9883-433b-a46b-68cead18deec" />

Ignitron Preset Tools:
----------------------

-you can put the "Ignitron Preset Tools.exe" in the root directory of your ignitron folder(/ignitron)

-To enable the ignitron pedal to pull current pedal presets, as well as streaming presets from the app, we have to modify 2 files in the ignitron firmware:


  -ignitron.ino - 3 strings                  in the main ignitron folder
  -SparkPresetControl.cpp - 1 string         in the /src folder

-add the following settings in /Ignitron/Ignitron.ino file to enable sending "LISTPRESETS" and "LISTBANKS" to trigger dumping presetlist over serial, 
-you can also just copy the 2 files ignitron.ino and SparkPresetControl.cpp into their respective folders, and modify the bits that are specific to your pedal(pins, leds, screen etc):

A. preset pulling setup  (edit /ignitron/ignitron.ino):
...................................................

1. ****add this line to the include librarys at start of file:

#include <LittleFS.h>


==========================================================================================================================

2. ****add this line right after "void loop() {"  :

handleSerialCommands();   // so it will react to LISTPRESETS 


==========================================================================================================================


3. ****add the following after the end of the file:
   

// === BEGIN: LISTPRESETS serial support =======================================

// Case-insensitive “.json” check  //mk
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
    if (c == '\r' || c == '\n' || c == '\t') continue; // keep it one line for the Python regex
    Serial.write(c);
  }
  Serial.println();
}

// List every *.json at the LittleFS root and print in the exact format your tool expects
static void listAllPresets() {
  // Optional markers so your Python tool can switch to an event-based end condition if you want later
  Serial.println("LISTPRESETS_START");

  File root = LittleFS.open("/");
  if (!root) {
    Serial.println("⚠️ Could not open LittleFS root");
    Serial.println("LISTPRESETS_DONE");
    return;
  }

  while (true) {
    File f = root.openNextFile();
    if (!f) break;                         // no more entries

    if (!f.isDirectory()) {
      const char *name = f.name();         // likely includes leading '/'
      if (name && hasJsonExt(name)) {
        // Keep wording EXACT—your Python regex looks for this line:
        Serial.print("Reading preset filename: ");
        Serial.println(name);               // e.g. "/MyPreset.json"

        // print the JSON on ONE LINE for robust parsing on the PC
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

      // Uppercase for simple matching
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
            if (c == '\r') continue; // normalize line endings
            Serial.write(c);
        }
        Serial.println("LISTBANKS_DONE");
        f.close();
    } else {
        Serial.println("⚠️ PresetList.txt not found");
        Serial.println("LISTBANKS_DONE");
    }
}

      // Add future commands here (e.g., "GETPRESET <name>", "DELETEPRESET <name>", etc.)
    } else {
      // Accumulate until newline
      buf += c;
      if (buf.length() > 256) {
        buf.remove(0, buf.length() - 256); // prevent runaway buffer
      }
    }
  }
}

// === END: LISTPRESETS serial support =========================================



======================================================================================================================================

B: spark app streaming setup (to enable saving presets as they are selected in the app):
.........................................................................................


1. **** edit SparkPresetControl.cpp located in /ignitron/src:

add:  

        // 🔧 Added for App Scraper
    Serial.println("received from app:");
    Serial.println(appReceivedPreset_.json.c_str());
	
	on the line after(around line 400) :


	void SparkPresetControl::updateFromSparkResponseAmpPreset(char *presetJson) {
    presetEditMode_ = PRESET_EDIT_STORE;
    appReceivedPreset_ = presetBuilder.getPresetFromJson(presetJson);
    DEBUG_PRINTLN("received from app:");
    DEBUG_PRINTLN(appReceivedPreset_.json.c_str());
	
	**insert here**

-------

****the final snippet should look like this:


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


---------------------------------------------------------------------------------------------------------------

then compile and flash to pedal.  enjoy
	
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Igniitron preset tools operation:
---------------------------------
OVERVIEW
______________

  -first of all, make sure your pedal can connect to your device running the spark app over BLE or SRL properly. 

   -pulling presets only works when the pedal is connected over usb to your computer in AMP mode (holding switch 1 when booting the
    ignitron pedal on. i recomend making sure it is already in amp mode when you connect the usb cable. when the specific tool starts connection, 
    you will need to be holding down switch 1 as well.

   -holding switch 1 down when the tool starts the connection, the pedal will do a reboot and you can release switch 1 when you see the screen do a 
   reboot.(should also indicate the bt connection on screen(SRL/BLE), but not necessarily the bluetooth symbol
   
   
--------------------------------------------------------------------------------------------   
   
    OPERATION
___________________

main menu
--------------

- open ignitron_preset_tools.exe.  it will open with 4 options.

   1- Preset Picker
   2- Preset Puller
   3- App Scraper
   4- Exit
   
   
1 - Preset Picker
------------------------
discription: use this tool load your /data folder of presets, organise all them in the gui, and export the 2 files to build your presets.




























