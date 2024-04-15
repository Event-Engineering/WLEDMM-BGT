#pragma once

#include "wled.h"
#include <Arduino.h>
#include <Adafruit_MCP23X08.h>


class Usermod_BGT : public Usermod {

  private:

    Adafruit_MCP23X08 mcp;

    bool mcpConnected = false;

    bool initDone = false;
    unsigned long lastTime = 0;
    int updateEvery = 50;
    int longPressTime = 2000;
    int goldTimeout = 20;

    bool goldMode = false;
    bool judge1 = false;
    bool judge2 = false;
    bool judge3 = false;
    bool judge4 = false;

    bool prevButtons[6] = {1,1,1,1,1,1};
    int resetTime = 0;
    int goldTime = 0;

    int goldenPreset = 20;
    int resetPreset = 50;
    int shutdownPreset = 99;

    int goldenGPIOOutputPin = 0;

  public:

    void setup() {

        mcpConnected = mcp.begin_I2C();
        if (mcpConnected) {
            Serial.print("Connected to MCP Switch Interface");
        } else {
            Serial.print("Not Connected to MCP Switch Interface");
        }
        serializeConfig(); // slow but forces a sync with the settings system
    }


    void loop() {
        if (millis() - lastTime < updateEvery || strip.isUpdating() || !mcpConnected) return;
        lastTime = millis();

        for (int b = 0; b<6; b++) {
            bool reading = mcp.digitalRead(b);
            if (reading != prevButtons[b] && !reading) {
                if (b < 4) {
                    handleJudgeButton(b);
                } else if (b == 4) {
                    goldTime = lastTime;
                    handleGoldenButton();
                } else {
                    resetTime = lastTime;
                    handleReset();
                }  
            }
            if (!reading && b == 5 && resetTime != 0 && lastTime - resetTime > longPressTime) {
                handleShutdown();
                 resetTime = 0;
            }
            prevButtons[b] = reading;
        }

        if (goldMode && lastTime-goldTime > (goldTimeout * 1000)) {
            Serial.println("Gold timeout");
            handleReset();
        }

    }


    void handleJudgeButton(int button) {
        Serial.print("Judge "); Serial.print(button + 1); Serial.println(" pressed!");
        if (!goldMode) {
            switch (button) {
                case 0:
                    judge1 = !judge1;
                    break;
                case 1:
                    judge2 = !judge2;
                    break;
                case 2:
                    judge3 = !judge3;
                    break;
                case 3:
                    judge4 = !judge4;
                    break;
                default:
                    Serial.println("Unrecognised judge button");
                    break;
            }

        int newPreset = 0;
        if (judge1) { newPreset += 1; }
        if (judge2) { newPreset += 2; }
        if (judge3) { newPreset += 4; }
        if (judge4) { newPreset += 8; }

        if (newPreset == 0) newPreset = 50; // reset as all judges now off

        Serial.print("Loading BGT Preset: "); Serial.println(newPreset);
        applyPreset(newPreset);
        }
    }

    void handleGoldenButton() {
        Serial.print("Golden Buzzer! -- Loading Preset: "); Serial.println(goldenPreset);
        goldMode = true;
        applyPreset(goldenPreset);

        goldenGPIO(HIGH);
    }

    void handleReset() {
        Serial.print("Reset system -- Loading preset: "); Serial.println(resetPreset);
        goldMode = false;
        applyPreset(resetPreset);

        goldenGPIO(LOW);
    }

    void handleShutdown() {
        Serial.print("Shutting down -- Loading preset: "); Serial.println(shutdownPreset);
        applyPreset(shutdownPreset);

    }

    void goldenGPIO(bool output) {
        if (goldenGPIOOutputPin != 0) {
            Serial.print("Triggering Golden GPIO on pin: "); Serial.print(goldenGPIOOutputPin); Serial.println(output);
            pinMode(goldenGPIOOutputPin, OUTPUT);
            digitalWrite(goldenGPIOOutputPin, output);
        }
    }



    /*
     * addToConfig() can be used to add custom persistent settings to the cfg.json file in the "um" (usermod) object.
     * It will be called by WLED when settings are actually saved (for example, LED settings are saved)
     * If you want to force saving the current state, use serializeConfig() in your loop().
     * 
     * CAUTION: serializeConfig() will initiate a filesystem write operation.
     * It might cause the LEDs to stutter and will cause flash wear if called too often.
     * Use it sparingly and always in the loop, never in network callbacks!
     * 
     * addToConfig() will also not yet add your setting to one of the settings pages automatically.
     * To make that work you still have to add the setting to the HTML, xml.cpp and set.cpp manually.
     * 
     * I highly recommend checking out the basics of ArduinoJson serialization and deserialization in order to use custom settings!
     */
    void addToConfig(JsonObject& root) {
      JsonObject top   = root.createNestedObject("Britains Got Talent");

      top["Update Buttons Every"] = updateEvery; // help for Settings page
      top["Golden Buzzer Timeout"] = goldTimeout;
      top["Reset Button Long Press Time"] = longPressTime;

      top["Preset Number For Golden Buzzer"] = goldenPreset;
      top["Preset Number For Reset"] = resetPreset;
      top["Preset Number For Shutdown"] = shutdownPreset;

      top["Golden Buzzer GPIO Output Pin"] = goldenGPIOOutputPin;
    }

    /*
     * readFromConfig() can be used to read back the custom settings you added with addToConfig().
     * This is called by WLED when settings are loaded (currently this only happens once immediately after boot)
     * 
     * readFromConfig() is called BEFORE setup(). This means you can use your persistent values in setup() (e.g. pin assignments, buffer sizes),
     * but also that if you want to write persistent values to a dynamic buffer, you'd need to allocate it here instead of in setup.
     * If you don't know what that is, don't fret. It most likely doesn't affect your use case :)
     */
    bool readFromConfig(JsonObject& root) {
        JsonObject top = root["Britains Got Talent"];

      updateEvery = top["Update Buttons Every"] | updateEvery;
      goldTimeout = top["Golden Buzzer Timeout"] | goldTimeout;
      longPressTime = top["Reset Button Long Press Time"] | longPressTime;

      goldenPreset = top["Preset Number For Golden Buzzer"] | goldenPreset;
      resetPreset = top["Preset Number For Reset"] | resetPreset;
      shutdownPreset = top["Preset Number For Shutdown"] | shutdownPreset;

      goldenGPIOOutputPin = top["Golden Buzzer GPIO Output Pin"] | goldenGPIOOutputPin;
    }

};