#pragma once
#include <Arduino.h>
#include <esp_camera.h>
#include <FS.h>

class CameraModule {
  public:
    bool begin();
    bool takePhoto(fs::FS &fs, const String &path);
    camera_fb_t *capture();
    void release(camera_fb_t *fb);
};

