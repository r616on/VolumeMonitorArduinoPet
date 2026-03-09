#include "PresetDetector.h"

PresetDetector::PresetDetector(uint8_t pin) :
    _pin(pin),
    _minPulseDur(50),      // значения по умолчанию
    _maxPulseDur(300),
    _seriesTimeout(2500),
    _state(IDLE),
    _prevLevel(HIGH),
    _pulseActive(false),
    _pulseStart(0),
    _lastPulseEnd(0),
    _currentSeriesCount(0),
    _lastSeriesCount(0),
    _consecutiveMatches(0),
    _confirmedPreset(0)
{
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
        } else {                 // конец импульса (LOW -> HIGH)
            if (_pulseActive) {
                unsigned long duration = now - _pulseStart;
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
        if (now - _lastPulseEnd > _seriesTimeout) {
            _endSeries();
        }
    }
}

void PresetDetector::_endSeries() {
    if (_lastSeriesCount == 0) {
        // Первая завершённая серия
        _lastSeriesCount = _currentSeriesCount;
        _consecutiveMatches = 1;
    } else {
        if (_currentSeriesCount == _lastSeriesCount) {
            _consecutiveMatches++;
            if (_consecutiveMatches >= 2) {
                _confirmedPreset = _currentSeriesCount;
                // Здесь можно добавить вызов callback'а или просто сохранить
            }
        } else {
            _lastSeriesCount = _currentSeriesCount;
            _consecutiveMatches = 1;
        }
    }
    _state = IDLE;
    _currentSeriesCount = 0;
}

int PresetDetector::getConfirmedPreset() const {
    return _confirmedPreset;
}

void PresetDetector::reset() {
    _state = IDLE;
    _currentSeriesCount = 0;
    _lastSeriesCount = 0;
    _consecutiveMatches = 0;
    _confirmedPreset = 0;
    _pulseActive = false;
    _prevLevel = digitalRead(_pin); // перечитываем текущее состояние
}

void PresetDetector::setPulseRange(unsigned long minDur, unsigned long maxDur) {
    _minPulseDur = minDur;
    _maxPulseDur = maxDur;
}

void PresetDetector::setTimeout(unsigned long seriesTimeout) {
    _seriesTimeout = seriesTimeout;
}