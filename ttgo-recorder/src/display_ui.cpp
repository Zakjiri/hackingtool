#include "display_ui.h"

bool DisplayUI::begin() {
    _tft.init();
    _tft.setRotation(0);
    _tft.fillScreen(TFT_BLACK);
    _tft.setTextColor(TFT_WHITE, TFT_BLACK);
    _tft.setTextFont(2);
    return true;
}

void DisplayUI::showBoot(const String &message) {
    _tft.fillScreen(TFT_BLACK);
    _tft.setTextDatum(MC_DATUM);
    _tft.drawString("TCamera Recorder", _tft.width() / 2, _tft.height() / 2 - 10);
    _tft.drawString(message, _tft.width() / 2, _tft.height() / 2 + 10);
    _lastUpdate = millis();
}

void DisplayUI::showStatus(const String &wifiMode, const String &ip, const String &timeStr, const String &battery, bool sdMounted) {
    if (millis() - _lastUpdate < 1000) {
        return;
    }
    _lastUpdate = millis();
    _tft.fillScreen(TFT_BLACK);
    _tft.setCursor(0, 0);
    _tft.printf("WiFi: %s\n", wifiMode.c_str());
    _tft.printf("IP: %s\n", ip.c_str());
    _tft.printf("Time: %s\n", timeStr.c_str());
    _tft.printf("Battery: %s\n", battery.c_str());
    _tft.printf("SD: %s\n", sdMounted ? "OK" : "Missing");
}

void DisplayUI::showRecording(bool recording, uint32_t seconds) {
    _tft.fillScreen(recording ? TFT_RED : TFT_BLACK);
    _tft.setTextColor(TFT_WHITE, recording ? TFT_RED : TFT_BLACK);
    _tft.setTextDatum(MC_DATUM);
    if (recording) {
        _tft.drawString("REC", _tft.width() / 2, _tft.height() / 2 - 10);
        _tft.drawString(String(seconds) + "s", _tft.width() / 2, _tft.height() / 2 + 10);
    } else {
        _tft.drawString("IDLE", _tft.width() / 2, _tft.height() / 2);
    }
}

void DisplayUI::showStreaming(const String &label) {
    _tft.fillScreen(TFT_BLUE);
    _tft.setTextDatum(MC_DATUM);
    _tft.setTextColor(TFT_WHITE, TFT_BLUE);
    _tft.drawString(label, _tft.width() / 2, _tft.height() / 2);
}

void DisplayUI::loop() {}
