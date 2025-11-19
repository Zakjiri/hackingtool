#include "power_manager.h"

bool PowerManager::begin(TwoWire &wire) {
    _wire = &wire;
    _wire->begin(18, 23); // Reuse camera I2C pins
    return true;
}

uint8_t PowerManager::readRegister(uint8_t reg) {
    if (_wire == nullptr) {
        return 0xFF;
    }
    _wire->beginTransmission(kIp5306Address);
    _wire->write(reg);
    if (_wire->endTransmission(false) != 0) {
        return 0xFF;
    }
    _wire->requestFrom(kIp5306Address, (uint8_t)1);
    if (_wire->available()) {
        return _wire->read();
    }
    return 0xFF;
}

int PowerManager::getBatteryPercent() {
    uint8_t level = readRegister(0x78);
    if (level == 0xFF) {
        return -1;
    }
    // Bits 3:0 map to the LED segments. Translate to percentage.
    if (level & 0x80) return 100;
    if (level & 0x40) return 75;
    if (level & 0x20) return 50;
    if (level & 0x10) return 25;
    return 10;
}

bool PowerManager::isCharging() {
    uint8_t status = readRegister(0x70);
    if (status == 0xFF) {
        return false;
    }
    return status & 0x08;
}
