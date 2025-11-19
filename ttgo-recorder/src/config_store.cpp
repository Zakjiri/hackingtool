#include "config_store.h"
#include <ArduinoJson.h>

static constexpr const char *kPrefsNamespace = "tcamera";
static constexpr const char *kConfigPath = "/config.json";

ConfigStore ConfigManager;

ConfigStore::ConfigStore() : _sd(nullptr), _sdAvailable(false) {}

bool ConfigStore::begin(fs::FS *sdFs) {
    _sd = sdFs;
    _sdAvailable = (_sd != nullptr);
    _prefs.begin(kPrefsNamespace, false);
    return true;
}

bool ConfigStore::load(Config &cfg) {
    if (loadFromSd(cfg)) {
        return true;
    }
    return loadFromNvs(cfg);
}

bool ConfigStore::save(const Config &cfg) {
    bool ok = saveToSd(cfg);
    if (!ok) {
        ok = saveToNvs(cfg);
    }
    return ok;
}

bool ConfigStore::loadFromSd(Config &cfg) {
    if (!_sdAvailable) {
        return false;
    }
    if (!_sd->exists(kConfigPath)) {
        return false;
    }
    File f = _sd->open(kConfigPath, FILE_READ);
    if (!f) {
        return false;
    }
    StaticJsonDocument<512> doc;
    DeserializationError err = deserializeJson(doc, f);
    f.close();
    if (err) {
        return false;
    }
    if (doc.containsKey("wifi_mode")) cfg.wifi_mode = doc["wifi_mode"].as<String>();
    if (doc.containsKey("wifi_ssid")) cfg.wifi_ssid = doc["wifi_ssid"].as<String>();
    if (doc.containsKey("wifi_password")) cfg.wifi_password = doc["wifi_password"].as<String>();
    if (doc.containsKey("device_name")) cfg.device_name = doc["device_name"].as<String>();
    if (doc.containsKey("vox_threshold")) cfg.vox_threshold = doc["vox_threshold"].as<uint16_t>();
    if (doc.containsKey("ntp_server")) cfg.ntp_server = doc["ntp_server"].as<String>();
    if (doc.containsKey("timezone_offset")) cfg.timezone_offset = doc["timezone_offset"].as<int>();
    return true;
}

bool ConfigStore::saveToSd(const Config &cfg) {
    if (!_sdAvailable) {
        return false;
    }
    File f = _sd->open(kConfigPath, FILE_WRITE);
    if (!f) {
        return false;
    }
    f.seek(0);
    f.truncate(0);
    StaticJsonDocument<512> doc;
    doc["wifi_mode"] = cfg.wifi_mode;
    doc["wifi_ssid"] = cfg.wifi_ssid;
    doc["wifi_password"] = cfg.wifi_password;
    doc["device_name"] = cfg.device_name;
    doc["vox_threshold"] = cfg.vox_threshold;
    doc["ntp_server"] = cfg.ntp_server;
    doc["timezone_offset"] = cfg.timezone_offset;
    serializeJsonPretty(doc, f);
    f.close();
    return true;
}

bool ConfigStore::loadFromNvs(Config &cfg) {
    cfg.wifi_ssid = _prefs.getString("ssid", cfg.wifi_ssid);
    cfg.wifi_password = _prefs.getString("pass", cfg.wifi_password);
    cfg.device_name = _prefs.getString("name", cfg.device_name);
    cfg.vox_threshold = _prefs.getUShort("vox", cfg.vox_threshold);
    cfg.timezone_offset = _prefs.getInt("tz", cfg.timezone_offset);
    cfg.ntp_server = _prefs.getString("ntp", cfg.ntp_server);
    cfg.wifi_mode = _prefs.getString("wmode", cfg.wifi_mode);
    return true;
}

bool ConfigStore::saveToNvs(const Config &cfg) {
    _prefs.putString("ssid", cfg.wifi_ssid);
    _prefs.putString("pass", cfg.wifi_password);
    _prefs.putString("name", cfg.device_name);
    _prefs.putUShort("vox", cfg.vox_threshold);
    _prefs.putInt("tz", cfg.timezone_offset);
    _prefs.putString("ntp", cfg.ntp_server);
    _prefs.putString("wmode", cfg.wifi_mode);
    return true;
}

bool config_load(Config &cfg) {
    return ConfigManager.load(cfg);
}

bool config_save(const Config &cfg) {
    return ConfigManager.save(cfg);
}
