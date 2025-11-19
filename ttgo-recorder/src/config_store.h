#pragma once
#include <Arduino.h>
#include <FS.h>
#include <Preferences.h>

struct Config {
    String wifi_mode = "auto";
    String wifi_ssid = "";
    String wifi_password = "";
    String device_name = "TCamera Recorder";
    uint16_t vox_threshold = 2000; // Approximate I2S amplitude threshold
    String ntp_server = "pool.ntp.org";
    int timezone_offset = 0; // minutes offset from UTC
};

class ConfigStore {
  public:
    ConfigStore();
    bool begin(fs::FS *sdFs);
    bool load(Config &cfg);
    bool save(const Config &cfg);
    bool isSdAvailable() const { return _sdAvailable; }
  private:
    bool loadFromSd(Config &cfg);
    bool saveToSd(const Config &cfg);
    bool loadFromNvs(Config &cfg);
    bool saveToNvs(const Config &cfg);
    fs::FS *_sd;
    Preferences _prefs;
    bool _sdAvailable;
};

extern ConfigStore ConfigManager;
bool config_load(Config &cfg);
bool config_save(const Config &cfg);
