#include "camera_module.h"
#include <SD.h>

bool CameraModule::begin() {
    camera_config_t config;
    config.ledc_channel = LEDC_CHANNEL_0;
    config.ledc_timer = LEDC_TIMER_0;
    config.pin_d0 = 36;
    config.pin_d1 = 37;
    config.pin_d2 = 38;
    config.pin_d3 = 39;
    config.pin_d4 = 35;
    config.pin_d5 = 26;
    config.pin_d6 = 13;
    config.pin_d7 = 34;
    config.pin_xclk = 4;
    config.pin_pclk = 25;
    config.pin_vsync = 5;
    config.pin_href = 27;
    config.pin_sscb_sda = 18;
    config.pin_sscb_scl = 23;
    config.pin_pwdn = -1;
    config.pin_reset = -1;
    config.xclk_freq_hz = 20000000;
    config.pixel_format = PIXFORMAT_JPEG;
    if (psramFound()) {
        config.frame_size = FRAMESIZE_SVGA;
        config.jpeg_quality = 12;
        config.fb_count = 2;
    } else {
        config.frame_size = FRAMESIZE_VGA;
        config.jpeg_quality = 15;
        config.fb_count = 1;
    }

    esp_err_t err = esp_camera_init(&config);
    if (err != ESP_OK) {
        Serial.printf("[CAMERA] Init failed 0x%X\n", err);
        return false;
    }
    sensor_t *s = esp_camera_sensor_get();
    s->set_framesize(s, FRAMESIZE_VGA);
    return true;
}

camera_fb_t *CameraModule::capture() {
    return esp_camera_fb_get();
}

void CameraModule::release(camera_fb_t *fb) {
    if (fb) {
        esp_camera_fb_return(fb);
    }
}

bool CameraModule::takePhoto(fs::FS &fs, const String &path) {
    camera_fb_t *fb = capture();
    if (!fb) {
        return false;
    }
    File file = fs.open(path, FILE_WRITE);
    if (!file) {
        release(fb);
        return false;
    }
    file.write(fb->buf, fb->len);
    file.close();
    release(fb);
    return true;
}
