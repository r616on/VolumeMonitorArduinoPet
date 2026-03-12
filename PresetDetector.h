#ifndef PRESET_DETECTOR_H
#define PRESET_DETECTOR_H

#include <Arduino.h>

class PresetDetector {
public:
  // Геттеры для отладки
  int getCurrentSeriesCount() const { return _currentSeriesCount; }
  int getState() const { return _state; }
  unsigned long getLastPulseEnd() const { return _lastPulseEnd; }

  // Конструктор: принимает пин, к которому подключена оптопара
  PresetDetector(uint8_t pin);

  // Должен вызываться в loop(), обрабатывает логику детектирования
  void update();

  // Возвращает последний подтверждённый номер пресета (0, если не подтверждён)
  int getConfirmedPreset() const;

  // Сброс состояния детектора
  void reset();

  // Настройка параметров
  void setPulseRange(unsigned long minDur, unsigned long maxDur);
  void setTimeout(unsigned long seriesTimeout);

private:
  uint8_t _pin;
  unsigned long _minPulseDur;
  unsigned long _maxPulseDur;
  unsigned long _seriesTimeout;

  // Состояние конечного автомата
  enum State { IDLE, IN_SERIES } _state;

  // Переменные для измерения импульсов
  int _prevLevel;
  bool _pulseActive;
  unsigned long _pulseStart;
  unsigned long _lastPulseEnd;

  // Счётчики серий
  int _currentSeriesCount;
  int _confirmedPreset;  // теперь подтверждается сразу

  // Вспомогательный метод для завершения серии
  void _endSeries();
};

#endif