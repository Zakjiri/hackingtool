#include <Arduino.h>
#include <WiFi.h>
#include <SPI.h>
#include <SD.h>
#include <LittleFS.h>
#include <time.h>
#include "audio_recorder.h"
#include "camera_module.h"
#include "display_ui.h"
#include "web_server.h"
#include "config_store.h"
#include "power_manager.h"

static constexpr uint8_t PIN_SD_CS = 0;
static constexpr uint8_t PIN_SD_SCK = 21;
static constexpr uint8_t PIN_SD_MISO = 22;
static constexpr uint8_t PIN_SD_MOSI = 19;

AudioRecorder gAudio;
CameraModule gCamera;
DisplayUI gDisplay;
WebServerModule gWeb;
Config gConfig;
PowerManager gPower;
SPIClass spiSd(HSPI);
bool gSdMounted = false;
bool gWifiConnected = false;

String formatTimeString() {
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo, 5)) {
        return "No time";
    }
    char buffer[32];
    strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", &timeinfo);
    return String(buffer);
}

void ensureFolders() {
    if (!gSdMounted) return;
    if (!SD.exists("/audio")) {
        SD.mkdir("/audio");
    }
    if (!SD.exists("/photos")) {
        SD.mkdir("/photos");
    }
}

void connectWifi() {
    if (gConfig.wifi_ssid.length() == 0) {
        WiFi.mode(WIFI_AP);
        WiFi.softAP("TCAMERA-RECORDER", "12345678");
        return;
    }
    WiFi.mode(WIFI_AP_STA);
    WiFi.softAP("TCAMERA-RECORDER", "12345678");
    WiFi.begin(gConfig.wifi_ssid.c_str(), gConfig.wifi_password.c_str());
    Serial.printf("[WIFI] Connecting to %s\n", gConfig.wifi_ssid.c_str());
    uint32_t start = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - start < 20000) {
        delay(250);
        Serial.print('.');
    }
    gWifiConnected = WiFi.status() == WL_CONNECTED;
    Serial.println();
    if (gWifiConnected) {
        Serial.printf("[WIFI] Connected, IP: %s\n", WiFi.localIP().toString().c_str());
        configTime(gConfig.timezone_offset * 60, 0, gConfig.ntp_server.c_str(), "pool.ntp.org", "time.nist.gov");
    } else {
        Serial.println("[WIFI] Failed, AP only");
    }
}

void setup() {
    Serial.begin(115200);
    delay(200);
    Serial.println("TCamera Recorder boot");

    gDisplay.begin();
    gDisplay.showBoot("Booting...");

    spiSd.begin(PIN_SD_SCK, PIN_SD_MISO, PIN_SD_MOSI, PIN_SD_CS);
    gSdMounted = SD.begin(PIN_SD_CS, spiSd, 4000000);
    if (!gSdMounted) {
        Serial.println("[SD] Mount failed");
        ConfigManager.begin(nullptr);
    } else {
        ConfigManager.begin(&SD);
        ensureFolders();
    }

    config_load(gConfig);

    gPower.begin();
    gCamera.begin();
    gAudio.begin(16000);
    gAudio.setVoxThreshold(gConfig.vox_threshold);
    gAudio.enableVox(true);

    connectWifi();
    gWeb.begin(&gAudio, &gCamera, &gDisplay, &gConfig, &gPower);
}

void loop() {
    gAudio.loop();
    gWeb.loop();
    gWifiConnected = WiFi.status() == WL_CONNECTED;

    if (gAudio.isRecording()) {
        gDisplay.showRecording(true, gAudio.recordingSeconds());
    } else if (gWeb.isAudioStreaming()) {
        gDisplay.showStreaming("Audio Stream");
    } else if (gWeb.isVideoStreaming()) {
        gDisplay.showStreaming("Video Stream");
    } else {
        String wifiStatus = gWifiConnected ? String("STA ") + WiFi.SSID() : "AP";
        String ip = gWifiConnected ? WiFi.localIP().toString() : WiFi.softAPIP().toString();
        String timeStr = formatTimeString();
        int battery = gPower.getBatteryPercent();
        String batteryStr = battery < 0 ? "?" : String(battery) + "%";
        gDisplay.showStatus(wifiStatus, ip, timeStr, batteryStr, gSdMounted);
    }

    delay(10);
}
