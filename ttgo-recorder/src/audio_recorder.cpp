#include "audio_recorder.h"
#include <driver/i2s.h>
#include <SD.h>
#include <cstring>

static constexpr gpio_num_t PIN_I2S_BCK = GPIO_NUM_14;
static constexpr gpio_num_t PIN_I2S_WS  = GPIO_NUM_32;
static constexpr gpio_num_t PIN_I2S_SD  = GPIO_NUM_33;
static constexpr i2s_port_t I2S_PORT = I2S_NUM_0;

bool AudioRecorder::begin(uint32_t sampleRate) {
    _sampleRate = sampleRate;
    return initI2S(sampleRate);
}

bool AudioRecorder::initI2S(uint32_t sampleRate) {
    i2s_config_t config = {
        .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
        .sample_rate = (int)sampleRate,
        .bits_per_sample = I2S_BITS_PER_SAMPLE_32BIT,
        .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
        .communication_format = I2S_COMM_FORMAT_I2S | I2S_COMM_FORMAT_I2S_MSB,
        .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
        .dma_buf_count = 8,
        .dma_buf_len = 256,
        .use_apll = false,
        .tx_desc_auto_clear = false,
        .fixed_mclk = 0
    };

    i2s_pin_config_t pinConfig = {
        .bck_io_num = PIN_I2S_BCK,
        .ws_io_num = PIN_I2S_WS,
        .data_out_num = I2S_PIN_NO_CHANGE,
        .data_in_num = PIN_I2S_SD,
        .mck_io_num = I2S_PIN_NO_CHANGE
    };

    i2s_driver_uninstall(I2S_PORT);
    if (i2s_driver_install(I2S_PORT, &config, 0, nullptr) != ESP_OK) {
        return false;
    }
    if (i2s_set_pin(I2S_PORT, &pinConfig) != ESP_OK) {
        return false;
    }
    i2s_zero_dma_buffer(I2S_PORT);
    return true;
}

bool AudioRecorder::startRecording(const String &path) {
    if (_recording) {
        stopRecording();
    }
    File file = SD.open(path, FILE_WRITE);
    if (!file) {
        return false;
    }
    writeWavHeader(file, _sampleRate);
    _file = file;
    _recording = true;
    _dataBytes = 0;
    _startMillis = millis();
    _silenceSamples = 0;
    return true;
}

void AudioRecorder::stopRecording() {
    if (!_recording) {
        return;
    }
    finalizeWav(_file);
    _file.close();
    _recording = false;
}

void AudioRecorder::writeWavHeader(File &file, uint32_t sampleRate) {
    const uint16_t channels = 1;
    const uint16_t bitsPerSample = 16;
    uint32_t byteRate = sampleRate * channels * bitsPerSample / 8;
    uint16_t blockAlign = channels * bitsPerSample / 8;
    file.seek(0);
    file.write((const uint8_t *)"RIFF", 4);
    uint32_t chunkSize = 36;
    file.write((uint8_t *)&chunkSize, 4);
    file.write((const uint8_t *)"WAVE", 4);
    file.write((const uint8_t *)"fmt ", 4);
    uint32_t subChunk1Size = 16;
    file.write((uint8_t *)&subChunk1Size, 4);
    uint16_t audioFormat = 1;
    file.write((uint8_t *)&audioFormat, 2);
    file.write((uint8_t *)&channels, 2);
    file.write((uint8_t *)&sampleRate, 4);
    file.write((uint8_t *)&byteRate, 4);
    file.write((uint8_t *)&blockAlign, 2);
    file.write((uint8_t *)&bitsPerSample, 2);
    file.write((const uint8_t *)"data", 4);
    uint32_t subChunk2Size = 0;
    file.write((uint8_t *)&subChunk2Size, 4);
}

void AudioRecorder::finalizeWav(File &file) {
    file.seek(4);
    uint32_t chunkSize = 36 + _dataBytes;
    file.write((uint8_t *)&chunkSize, 4);
    file.seek(40);
    uint32_t subChunk2Size = _dataBytes;
    file.write((uint8_t *)&subChunk2Size, 4);
}

void AudioRecorder::loop() {
    if (!_recording) {
        return;
    }
    static int32_t rawBuffer[256];
    size_t bytesRead = 0;
    if (i2s_read(I2S_PORT, (void *)rawBuffer, sizeof(rawBuffer), &bytesRead, 10 / portTICK_PERIOD_MS) != ESP_OK) {
        return;
    }
    size_t samples = bytesRead / sizeof(int32_t);
    static int16_t converted[256];
    for (size_t i = 0; i < samples; ++i) {
        converted[i] = (int16_t)(rawBuffer[i] >> 11);
    }
    handleVox(converted, samples, samples * sizeof(int16_t));
}

void AudioRecorder::handleVox(int16_t *samples, size_t count, size_t bytes) {
    if (!_recording) {
        return;
    }
    bool allowed = true;
    if (_voxEnabled) {
        uint32_t peak = 0;
        for (size_t i = 0; i < count; ++i) {
            peak = max<uint32_t>(peak, abs(samples[i]));
        }
        if (peak < _voxThreshold) {
            _silenceSamples += count;
            if (_silenceSamples > _sampleRate / 2) {
                allowed = false;
            }
        } else {
            _silenceSamples = 0;
        }
    }
    if (allowed) {
        _file.write((uint8_t *)samples, bytes);
        _dataBytes += bytes;
    }
}

uint32_t AudioRecorder::recordingSeconds() const {
    if (!_recording) {
        return 0;
    }
    return (millis() - _startMillis) / 1000;
}

size_t AudioRecorder::readRaw(int16_t *dest, size_t maxSamples) {
    static int32_t raw[256];
    size_t bytesNeeded = min(maxSamples, (size_t)256) * sizeof(int32_t);
    size_t bytesRead = 0;
    if (i2s_read(I2S_PORT, (void *)raw, bytesNeeded, &bytesRead, 20 / portTICK_PERIOD_MS) != ESP_OK) {
        return 0;
    }
    size_t samples = bytesRead / sizeof(int32_t);
    for (size_t i = 0; i < samples; ++i) {
        dest[i] = (int16_t)(raw[i] >> 11);
    }
    return samples;
}

size_t AudioRecorder::fillStreamBuffer(uint8_t *buffer, size_t maxLen, size_t index) {
    if (index == 0) {
        if (maxLen < 44) {
            return 0;
        }
        // Compose WAV header in memory
        const uint16_t channels = 1;
        const uint16_t bitsPerSample = 16;
        uint32_t byteRate = _sampleRate * channels * bitsPerSample / 8;
        uint16_t blockAlign = channels * bitsPerSample / 8;
        memcpy(buffer, "RIFF", 4);
        uint32_t chunkSize = 0xFFFFFFFF;
        memcpy(buffer + 4, &chunkSize, 4);
        memcpy(buffer + 8, "WAVEfmt ", 8);
        uint32_t subChunk1Size = 16;
        memcpy(buffer + 16, &subChunk1Size, 4);
        uint16_t audioFormat = 1;
        memcpy(buffer + 20, &audioFormat, 2);
        memcpy(buffer + 22, &channels, 2);
        memcpy(buffer + 24, &_sampleRate, 4);
        memcpy(buffer + 28, &byteRate, 4);
        memcpy(buffer + 32, &blockAlign, 2);
        memcpy(buffer + 34, &bitsPerSample, 2);
        memcpy(buffer + 36, "data", 4);
        uint32_t dataSize = 0xFFFFFFFF;
        memcpy(buffer + 40, &dataSize, 4);
        return 44;
    }
    size_t samples = maxLen / sizeof(int16_t);
    size_t actual = readRaw((int16_t *)buffer, samples);
    return actual * sizeof(int16_t);
}
