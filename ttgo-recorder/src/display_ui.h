#pragma once
#include <Arduino.h>
#include <TFT_eSPI.h>

class DisplayUI {
  public:
    bool begin();
    void showBoot(const String &message);
    void showStatus(const String &wifiMode, const String &ip, const String &timeStr, const String &battery, bool sdMounted);
    void showRecording(bool recording, uint32_t seconds);
    void showStreaming(const String &label);
    void loop();
  private:
    TFT_eSPI _tft;
    uint32_t _lastUpdate = 0;
};
