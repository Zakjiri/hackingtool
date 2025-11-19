#pragma once
#include <Arduino.h>
#include <Wire.h>

class PowerManager {
  public:
    bool begin(TwoWire &wire = Wire);
    int getBatteryPercent();
    bool isCharging();
  private:
    uint8_t readRegister(uint8_t reg);
    TwoWire *_wire = nullptr;
    static constexpr uint8_t kIp5306Address = 0x75;
};
