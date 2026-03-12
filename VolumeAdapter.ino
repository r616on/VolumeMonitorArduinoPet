#include <Wire.h>
#include <ArduinoJson.h>
#include "PresetDetector.h"
#include "PotentiometerController.h"


#define BUTTON_PIN 9  // пин оптопары
unsigned long pressStartTime = 0;
bool buttonActive = false;

// Создаём объект детектора для пина A3
PresetDetector presetDetector(A3);

// Для отслеживания изменений подтверждённого пресета
int lastConfirmedPreset = 1;

#define POT_MAX_VALUE 255  // Максимальное значение потенциометра (0-255)


// --- Символы обрамления сообщений ---
#define MSG_START '['
#define MSG_END ']'


// --- Функция отправки JSON ответа с обрамлением и переводом строки ---
void sendResponse(JsonDocument& doc) {
  Serial.print(MSG_START);
  serializeJson(doc, Serial);
  Serial.print(MSG_END);
  Serial.println();  // Добавляет \n, завершая сообщение
}


// --- Парсинг JSON команды (строка без обрамления) ---
void parseCommand(String jsonString) {
  StaticJsonDocument<256> doc;
  DeserializationError error = deserializeJson(doc, jsonString);

  if (error) {
    StaticJsonDocument<128> errorDoc;
    errorDoc["status"] = "error";
    errorDoc["message"] = "Invalid JSON";
    errorDoc["detail"] = error.c_str();
    sendResponse(errorDoc);
    return;
  }

  const char* command = doc["command"];

  // Обработка команд
  if (strcmp(command, "set_volume") == 0) {
    int value = doc["value"] | -1;
    if (value >= 0 && value <= POT_MAX_VALUE) {
      bool success = setVolume(value);  // вызов функции из модуля
      StaticJsonDocument<128> responseDoc;
      if (success) {
        responseDoc["status"] = "success";
        responseDoc["command"] = "set_volume";
        responseDoc["volume"] = value;
      } else {
        responseDoc["status"] = "error";
        responseDoc["message"] = "I2C communication failed (volume)";
      }
      sendResponse(responseDoc);
    } else {
      StaticJsonDocument<128> errorDoc;
      errorDoc["status"] = "error";
      errorDoc["message"] = "value must be 0-255";
      errorDoc["received"] = value;
      sendResponse(errorDoc);
    }
  }

  else if (strcmp(command, "set_bass_level") == 0) {
    int value = doc["value"] | -1;
    if (value >= 0 && value <= POT_MAX_VALUE) {
      bool success = setBassLevel(value);
      StaticJsonDocument<128> responseDoc;
      if (success) {
        responseDoc["status"] = "success";
        responseDoc["command"] = "set_bass_level";
        responseDoc["value"] = value;
      } else {
        responseDoc["status"] = "error";
        responseDoc["message"] = "I2C communication failed (value)";
      }
      sendResponse(responseDoc);
    } else {
      StaticJsonDocument<128> errorDoc;
      errorDoc["status"] = "error";
      errorDoc["message"] = "value must be 0-255";
      errorDoc["received"] = value;
      sendResponse(errorDoc);
    }
  }

  else if (strcmp(command, "change_preset") == 0) {
    // Запуск имитации нажатия
    digitalWrite(BUTTON_PIN, HIGH);
    pressStartTime = millis();
    buttonActive = true;

    // Формирование ответа
    StaticJsonDocument<128> responseDoc;
    responseDoc["status"] = "success";
    responseDoc["command"] = "change_preset";
    responseDoc["message"] = "Button press simulated (500ms)";
    sendResponse(responseDoc);
  }

  else if (strcmp(command, "get_preset") == 0) {
    // Формирование ответа
    StaticJsonDocument<128> eventDoc;
    eventDoc["command"] = "preset_changed";
    eventDoc["value"] = lastConfirmedPreset;
    sendResponse(eventDoc);
  }

  else if (strcmp(command, "ping") == 0) {
    StaticJsonDocument<128> responseDoc;
    responseDoc["status"] = "success";
    responseDoc["command"] = "pong";
    responseDoc["device"] = "Volume_Adapter";
    responseDoc["volume_range"] = "0-255";
    responseDoc["bass_range"] = "0-255";
    sendResponse(responseDoc);
  } else {
    StaticJsonDocument<128> errorDoc;
    errorDoc["status"] = "error";
    errorDoc["message"] = "Unknown command";
    errorDoc["received_command"] = command;
    sendResponse(errorDoc);
  }
}

// --- Setup ---
void setup() {
  Serial.begin(115200);
  Wire.begin();
  pinMode(BUTTON_PIN, OUTPUT);
  digitalWrite(BUTTON_PIN, LOW);


  delay(100);

  // Приветственное сообщение
  StaticJsonDocument<128> welcomeDoc;
  welcomeDoc["status"] = "ready";
  welcomeDoc["device"] = "Volume Adapter";
  welcomeDoc["protocol"] = "JUDI";
  welcomeDoc["volume_range"] = "0-255";
  welcomeDoc["bass_range"] = "0-255";

  sendResponse(welcomeDoc);
  delay(100);

  // Устанавливаем начальные значения (например, 1 для избежания нуля, если нужно)
  setVolume(1);
  delay(10);
}

// --- Loop с чтением строк до '\n' ---
void loop() {
  unsigned long now = millis();

  // 1. Имитация нажатия на D9
  if (buttonActive && (now - pressStartTime >= 500)) {
    digitalWrite(BUTTON_PIN, LOW);
    buttonActive = false;
  }


  // 2. Обновление детектора пресета
  presetDetector.update();
  // 3. Проверка, изменился ли подтверждённый пресет
  int currentPreset = presetDetector.getConfirmedPreset();
  if (currentPreset != lastConfirmedPreset && currentPreset != 0) {
    StaticJsonDocument<128> eventDoc;
    eventDoc["command"] = "preset_changed";
    eventDoc["value"] = currentPreset;
    sendResponse(eventDoc);

    lastConfirmedPreset = currentPreset;
  }

  if (Serial.available() > 0) {
    String input = Serial.readStringUntil('\n');  // читаем до \n, сам \n не включается
    input.trim();                                 // удаляем пробельные символы по краям (включая \r, если есть)

    if (input.length() == 0) return;

    // Проверяем, что строка начинается с '[' и заканчивается ']'
    if (input.startsWith(String(MSG_START)) && input.endsWith(String(MSG_END))) {
      String content = input.substring(1, input.length() - 1);
      parseCommand(content);
    } else {
      StaticJsonDocument<128> errorDoc;
      errorDoc["status"] = "error";
      errorDoc["message"] = "Message must be enclosed in [ ]";
      sendResponse(errorDoc);
    }
  }
}