#include "PresetDetector.h"

PresetDetector::PresetDetector(uint8_t pin)
  : _pin(pin),
    _minPulseDur(100),    // минимум 100 мс (для импульсов ~150 мс)
    _maxPulseDur(200),    // максимум 200 мс
    _seriesTimeout(400),  // таймаут между сериями
    _state(IDLE),
    _prevLevel(HIGH),
    _pulseActive(false),
    _pulseStart(0),
    _lastPulseEnd(0),
    _currentSeriesCount(0),
    _confirmedPreset(0) {
  pinMode(_pin, INPUT_PULLUP);
  _prevLevel = digitalRead(_pin);
}

void PresetDetector::update() {
  unsigned long now = millis();
  int curLevel = digitalRead(_pin);

  // Обнаружение изменения уровня
  if (curLevel != _prevLevel) {
    if (curLevel == LOW) {  // начало импульса (HIGH -> LOW)
      _pulseActive = true;
      _pulseStart = now;
    } else {  // конец импульса (LOW -> HIGH)
      if (_pulseActive) {
        unsigned long duration = now - _pulseStart;

        // ВЫВОД ДЛИТЕЛЬНОСТИ ИМПУЛЬСА
        // Serial.print("Pulse END. Duration: ");
        // Serial.print(duration);
        // Serial.print("ms | ");

        if (duration >= _minPulseDur && duration <= _maxPulseDur) {
          // Валидный импульс
          if (_state == IDLE) {
            _state = IN_SERIES;
            _currentSeriesCount = 1;
          } else {
            _currentSeriesCount++;
          }
          _lastPulseEnd = now;
        } else {
          // Невалидный импульс – сброс серии
          _state = IDLE;
          _currentSeriesCount = 0;
        }
        _pulseActive = false;
      }
    }
    _prevLevel = curLevel;
  }

  // Проверка таймаута окончания серии
  if (_state == IN_SERIES && !_pulseActive) {
    unsigned long timeSinceLastPulse = now - _lastPulseEnd;
    
    if (timeSinceLastPulse > _seriesTimeout) {
      _endSeries();
    }
  }
}

void PresetDetector::_endSeries() {
  // Подтверждаем пресет сразу (если количество импульсов в разумных пределах)
  if (_currentSeriesCount >= 1 && _currentSeriesCount <= 10) {
    _confirmedPreset = _currentSeriesCount;
  }

  // Сбрасываем состояние для следующей серии
  _state = IDLE;
  _currentSeriesCount = 0;
}

int PresetDetector::getConfirmedPreset() const {
  return _confirmedPreset;
}

void PresetDetector::reset() {
  _state = IDLE;
  _currentSeriesCount = 0;
  _confirmedPreset = 0;
  _pulseActive = false;
  _prevLevel = digitalRead(_pin);
}

void PresetDetector::setPulseRange(unsigned long minDur, unsigned long maxDur) {
  _minPulseDur = minDur;
  _maxPulseDur = maxDur;
}

void PresetDetector::setTimeout(unsigned long seriesTimeout) {
  _seriesTimeout = seriesTimeout;
}