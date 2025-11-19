#pragma once
#include <Arduino.h>
#include <driver/i2s.h>
#include <FS.h>

class AudioRecorder {
  public:
    bool begin(uint32_t sampleRate = 16000);
    void loop();
    bool startRecording(const String &path);
    void stopRecording();
    bool isRecording() const { return _recording; }
    void enableVox(bool enable) { _voxEnabled = enable; }
    void setVoxThreshold(uint16_t threshold) { _voxThreshold = threshold; }
    uint32_t recordingSeconds() const;
    size_t readRaw(int16_t *dest, size_t maxSamples);
    size_t fillStreamBuffer(uint8_t *buffer, size_t maxLen, size_t index);
  private:
    bool initI2S(uint32_t sampleRate);
    void writeWavHeader(File &file, uint32_t sampleRate);
    void finalizeWav(File &file);
    void handleVox(int16_t *samples, size_t count, size_t bytes);
    bool _recording = false;
    bool _voxEnabled = false;
    uint16_t _voxThreshold = 2000;
    uint32_t _sampleRate = 16000;
    File _file;
    size_t _dataBytes = 0;
    uint32_t _startMillis = 0;
    uint32_t _silenceSamples = 0;
};
