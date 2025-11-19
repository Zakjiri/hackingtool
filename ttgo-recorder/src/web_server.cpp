#include "web_server.h"
#include <SD.h>
#include <ArduinoJson.h>
#include <WiFi.h>
#include <array>
#include <time.h>
#include <functional>

struct StreamContext {
    WebServerModule *instance;
    WiFiClient *client;
};

static String buildTimestampedName(const char *folder, const char *ext) {
    time_t now = time(nullptr);
    struct tm timeinfo;
    char buffer[32];
    if (localtime_r(&now, &timeinfo)) {
        strftime(buffer, sizeof(buffer), "%Y%m%d_%H%M%S", &timeinfo);
    } else {
        snprintf(buffer, sizeof(buffer), "%lu", (unsigned long)millis());
    }
    String path = folder;
    if (!path.endsWith("/")) {
        path += "/";
    }
    path += buffer;
    path += ext;
    return path;
}

WebServerModule::WebServerModule() : _server(80), _audio(nullptr), _camera(nullptr), _display(nullptr), _config(nullptr), _power(nullptr) {}

bool WebServerModule::begin(AudioRecorder *audio, CameraModule *camera, DisplayUI *display, Config *config, PowerManager *power) {
    _audio = audio;
    _camera = camera;
    _display = display;
    _config = config;
    _power = power;

    if (!LittleFS.begin(true)) {
        Serial.println("[WEB] Failed to mount LittleFS");
    }

    _server.on("/", HTTP_GET, [this]() {
        if (!serveStaticFile("/index.html", "text/html")) {
            _server.send(500, "text/plain", "Missing UI");
        }
    });
    _server.on("/app.js", HTTP_GET, [this]() {
        if (!serveStaticFile("/app.js", "application/javascript")) {
            _server.send(404, "text/plain", "not found");
        }
    });
    _server.on("/style.css", HTTP_GET, [this]() {
        if (!serveStaticFile("/style.css", "text/css")) {
            _server.send(404, "text/plain", "not found");
        }
    });

    _server.on("/api/status", HTTP_GET, std::bind(&WebServerModule::handleStatus, this));
    _server.on("/api/record/start", HTTP_POST, std::bind(&WebServerModule::handleRecordStart, this));
    _server.on("/api/record/stop", HTTP_POST, std::bind(&WebServerModule::handleRecordStop, this));
    _server.on("/api/photo", HTTP_POST, std::bind(&WebServerModule::handleTakePhoto, this));
    _server.on("/api/files", HTTP_GET, std::bind(&WebServerModule::handleListFiles, this));
    _server.on("/download", HTTP_GET, std::bind(&WebServerModule::handleDownload, this));
    _server.on("/api/file", HTTP_DELETE, std::bind(&WebServerModule::handleDelete, this));
    _server.on("/api/settings", HTTP_POST, std::bind(&WebServerModule::handleSettings, this));
    _server.on("/battery", HTTP_GET, std::bind(&WebServerModule::handleBattery, this));
    _server.on("/api/stream/audio/start", HTTP_POST, std::bind(&WebServerModule::handleAudioStreamStart, this));
    _server.on("/api/stream/audio/stop", HTTP_POST, std::bind(&WebServerModule::handleAudioStreamStop, this));
    _server.on("/api/stream/video/start", HTTP_POST, std::bind(&WebServerModule::handleVideoStreamStart, this));
    _server.on("/api/stream/video/stop", HTTP_POST, std::bind(&WebServerModule::handleVideoStreamStop, this));
    _server.on("/audio_stream", HTTP_GET, std::bind(&WebServerModule::handleAudioStream, this));
    _server.on("/video_stream", HTTP_GET, std::bind(&WebServerModule::handleVideoStream, this));
    _server.onNotFound(std::bind(&WebServerModule::handleNotFound, this));
    _server.begin();
    return true;
}

void WebServerModule::loop() {
    _server.handleClient();
}

bool WebServerModule::serveStaticFile(const char *path, const char *type) {
    File f = LittleFS.open(path, "r");
    if (!f) {
        return false;
    }
    _server.streamFile(f, type);
    f.close();
    return true;
}

void WebServerModule::handleNotFound() {
    _server.send(404, "text/plain", "Not found");
}

void WebServerModule::handleStatus() {
    StaticJsonDocument<768> doc;
    doc["device"] = _config ? _config->device_name : "TCamera";
    doc["recording"] = _audio && _audio->isRecording();
    doc["record_seconds"] = _audio ? _audio->recordingSeconds() : 0;
    doc["audio_streaming"] = _audioStreamingActive;
    doc["video_streaming"] = _videoStreamingActive;
    doc["battery"] = _power ? _power->getBatteryPercent() : -1;
    doc["wifi_ip"] = WiFi.localIP().toString();
    doc["ap_ip"] = WiFi.softAPIP().toString();
    doc["sd"] = SD.cardType() != CARD_NONE;
    if (_config) {
        JsonObject cfg = doc.createNestedObject("config");
        cfg["device_name"] = _config->device_name;
        cfg["wifi_ssid"] = _config->wifi_ssid;
        cfg["vox_threshold"] = _config->vox_threshold;
        cfg["ntp_server"] = _config->ntp_server;
        cfg["timezone_offset"] = _config->timezone_offset;
    }
    String json;
    serializeJson(doc, json);
    _server.send(200, "application/json", json);
}

void WebServerModule::handleRecordStart() {
    if (!_audio) {
        _server.send(500, "text/plain", "Audio missing");
        return;
    }
    String filename = buildTimestampedName("/audio", ".wav");
    if (!_audio->startRecording(filename)) {
        _server.send(500, "text/plain", "Failed to start");
        return;
    }
    _server.send(200, "text/plain", "Recording");
}

void WebServerModule::handleRecordStop() {
    if (_audio) {
        _audio->stopRecording();
    }
    _server.send(200, "text/plain", "Stopped");
}

void WebServerModule::handleTakePhoto() {
    if (!_camera) {
        _server.send(500, "text/plain", "Camera missing");
        return;
    }
    String path = buildTimestampedName("/photos", ".jpg");
    if (!_camera->takePhoto(SD, path)) {
        _server.send(500, "text/plain", "Failed");
        return;
    }
    _server.send(200, "text/plain", path);
}

static bool pathAllowed(const String &path) {
    return path.startsWith("/audio/") || path.startsWith("/photos/");
}

void WebServerModule::handleListFiles() {
    String type = _server.hasArg("type") ? _server.arg("type") : "audio";
    String folder = type == "photo" ? "/photos" : "/audio";
    File root = SD.open(folder);
    StaticJsonDocument<1024> doc;
    JsonArray arr = doc.createNestedArray("files");
    if (root) {
        File file = root.openNextFile();
        while (file) {
            if (!file.isDirectory()) {
                JsonObject obj = arr.createNestedObject();
                obj["name"] = file.name();
                obj["size"] = file.size();
                obj["modified"] = file.getLastWrite();
            }
            file = root.openNextFile();
        }
        root.close();
    }
    String json;
    serializeJson(doc, json);
    _server.send(200, "application/json", json);
}

void WebServerModule::handleDownload() {
    if (!_server.hasArg("path")) {
        _server.send(400, "text/plain", "Missing path");
        return;
    }
    String path = _server.arg("path");
    if (!pathAllowed(path)) {
        _server.send(403, "text/plain", "forbidden");
        return;
    }
    File f = SD.open(path, FILE_READ);
    if (!f) {
        _server.send(404, "text/plain", "missing");
        return;
    }
    _server.streamFile(f, "application/octet-stream");
    f.close();
}

void WebServerModule::handleDelete() {
    if (!_server.hasArg("path")) {
        _server.send(400, "text/plain", "Missing path");
        return;
    }
    String path = _server.arg("path");
    if (!pathAllowed(path)) {
        _server.send(403, "text/plain", "forbidden");
        return;
    }
    if (!SD.remove(path)) {
        _server.send(500, "text/plain", "delete failed");
        return;
    }
    _server.send(200, "text/plain", "deleted");
}

void WebServerModule::handleSettings() {
    if (!_server.hasArg("plain")) {
        _server.send(400, "text/plain", "no body");
        return;
    }
    StaticJsonDocument<512> doc;
    DeserializationError err = deserializeJson(doc, _server.arg("plain"));
    if (err) {
        _server.send(400, "text/plain", "json error");
        return;
    }
    if (_config) {
        if (doc.containsKey("device_name")) _config->device_name = doc["device_name"].as<String>();
        if (doc.containsKey("wifi_ssid")) _config->wifi_ssid = doc["wifi_ssid"].as<String>();
        if (doc.containsKey("wifi_password")) _config->wifi_password = doc["wifi_password"].as<String>();
        if (doc.containsKey("vox_threshold")) _config->vox_threshold = doc["vox_threshold"].as<uint16_t>();
        if (doc.containsKey("ntp_server")) _config->ntp_server = doc["ntp_server"].as<String>();
        if (doc.containsKey("timezone_offset")) _config->timezone_offset = doc["timezone_offset"].as<int>();
        config_save(*_config);
    }
    _server.send(200, "text/plain", "saved");
}

void WebServerModule::handleAudioStream() {
    _audioStreamingActive = true;
    WiFiClient client = _server.client();
    if (!client) {
        return;
    }
    StreamContext *ctx = new StreamContext{this, new WiFiClient(client)};
    xTaskCreatePinnedToCore(&WebServerModule::audioStreamTask, "audstr", 4096, ctx, 1, nullptr);
}

void WebServerModule::handleVideoStream() {
    _videoStreamingActive = true;
    WiFiClient client = _server.client();
    if (!client) {
        return;
    }
    StreamContext *ctx = new StreamContext{this, new WiFiClient(client)};
    xTaskCreatePinnedToCore(&WebServerModule::videoStreamTask, "vidstr", 6144, ctx, 1, nullptr);
}

void WebServerModule::audioStreamTask(void *param) {
    auto *ctx = static_cast<StreamContext *>(param);
    ctx->instance->runAudioStream(*ctx->client);
    delete ctx->client;
    delete ctx;
    vTaskDelete(nullptr);
}

void WebServerModule::videoStreamTask(void *param) {
    auto *ctx = static_cast<StreamContext *>(param);
    ctx->instance->runVideoStream(*ctx->client);
    delete ctx->client;
    delete ctx;
    vTaskDelete(nullptr);
}

void WebServerModule::runAudioStream(WiFiClient client) {
    if (!_audio) {
        client.stop();
        return;
    }
    client.println("HTTP/1.1 200 OK");
    client.println("Content-Type: audio/wav");
    client.println("Connection: close");
    client.println();
    uint8_t header[44];
    _audio->fillStreamBuffer(header, sizeof(header), 0);
    client.write(header, sizeof(header));
    std::array<int16_t, 256> buffer;
    while (client.connected() && _audioStreamingActive) {
        size_t samples = _audio->readRaw(buffer.data(), buffer.size());
        if (samples == 0) {
            delay(5);
            continue;
        }
        client.write((uint8_t *)buffer.data(), samples * sizeof(int16_t));
    }
    client.stop();
    _audioStreamingActive = false;
}

void WebServerModule::runVideoStream(WiFiClient client) {
    if (!_camera) {
        client.stop();
        return;
    }
    client.println("HTTP/1.1 200 OK");
    client.println("Content-Type: multipart/x-mixed-replace; boundary=frame");
    client.println("Connection: close");
    client.println();
    while (client.connected() && _videoStreamingActive) {
        camera_fb_t *fb = _camera->capture();
        if (!fb) {
            delay(10);
            continue;
        }
        client.printf("--frame\r\nContent-Type: image/jpeg\r\nContent-Length: %u\r\n\r\n", fb->len);
        client.write(fb->buf, fb->len);
        client.print("\r\n");
        _camera->release(fb);
        delay(100);
    }
    client.stop();
    _videoStreamingActive = false;
}

void WebServerModule::handleAudioStreamStart() {
    _audioStreamingActive = true;
    _server.send(200, "text/plain", "audio stream enabled");
}

void WebServerModule::handleAudioStreamStop() {
    _audioStreamingActive = false;
    _server.send(200, "text/plain", "audio stream disabled");
}

void WebServerModule::handleVideoStreamStart() {
    _videoStreamingActive = true;
    _server.send(200, "text/plain", "video stream enabled");
}

void WebServerModule::handleVideoStreamStop() {
    _videoStreamingActive = false;
    _server.send(200, "text/plain", "video stream disabled");
}

void WebServerModule::handleBattery() {
    StaticJsonDocument<128> doc;
    doc["percent"] = _power ? _power->getBatteryPercent() : -1;
    doc["charging"] = _power ? _power->isCharging() : false;
    String json;
    serializeJson(doc, json);
    _server.send(200, "application/json", json);
}
