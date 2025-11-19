#pragma once
#include <Arduino.h>
#include <WebServer.h>
#include <LittleFS.h>
#include "audio_recorder.h"
#include "camera_module.h"
#include "display_ui.h"
#include "config_store.h"
#include "power_manager.h"

class WebServerModule {
  public:
    WebServerModule();
    bool begin(AudioRecorder *audio, CameraModule *camera, DisplayUI *display, Config *config, PowerManager *power);
    void loop();
    void updateConfig(Config *config) { _config = config; }
    bool isAudioStreaming() const { return _audioStreamingActive; }
    bool isVideoStreaming() const { return _videoStreamingActive; }
  private:
    WebServer _server;
    AudioRecorder *_audio;
    CameraModule *_camera;
    DisplayUI *_display;
    Config *_config;
    PowerManager *_power;
    volatile bool _audioStreamingActive = false;
    volatile bool _videoStreamingActive = false;
    bool serveStaticFile(const char *path, const char *type);
    void handleStatus();
    void handleRecordStart();
    void handleRecordStop();
    void handleTakePhoto();
    void handleListFiles();
    void handleDownload();
    void handleDelete();
    void handleSettings();
    void handleAudioStream();
    void handleVideoStream();
    void handleAudioStreamStart();
    void handleAudioStreamStop();
    void handleVideoStreamStart();
    void handleVideoStreamStop();
    void handleBattery();
    void handleNotFound();
    void runAudioStream(WiFiClient client);
    void runVideoStream(WiFiClient client);
    static void audioStreamTask(void *param);
    static void videoStreamTask(void *param);
};
