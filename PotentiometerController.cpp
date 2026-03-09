#include "PotentiometerController.h"

// Определение глобальных переменных
int currentVolume = -1;
int currentBass = -1;

bool setPotValue(uint8_t deviceAddr, int value) {
  if (value < 0) value = 0;
  if (value > POT_MAX_VALUE) value = POT_MAX_VALUE;

  Wire.beginTransmission(deviceAddr);
  Wire.write(MCP4561_CMD_WRITE_WIPER0);
  Wire.write((uint8_t)value);
  return (Wire.endTransmission() == 0);
}

bool setPotValueMemory(uint8_t deviceAddr, int value) {
  if (value < 0) value = 0;
  if (value > POT_MAX_VALUE) value = POT_MAX_VALUE;

  // Запись в RAM
  Wire.beginTransmission(deviceAddr);
  Wire.write(MCP4561_CMD_WRITE_WIPER0);
  Wire.write((uint8_t)value);
  if (Wire.endTransmission() != 0) return false;

  // Запись в NV
  Wire.beginTransmission(deviceAddr);
  Wire.write(MCP4561_CMD_WRITE_NV_WIPER0);
  Wire.write((uint8_t)value);
  return (Wire.endTransmission() == 0);
}

bool setVolume(int value) {
  bool success = setPotValue(MCP4561_ADDR_VOLUME, value);
  if (success) currentVolume = value;
  return success;
}

bool setBassLevel(int value) {
  bool success = setPotValueMemory(MCP4561_ADDR_BASS, value);
  if (success) currentBass = value;
  return success;
}