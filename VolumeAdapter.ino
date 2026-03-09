#include <Wire.h>
#include <ArduinoJson.h>

// --- Адреса и константы MCP4561 ---
#define MCP4561_ADDR_VOLUME 0x2E  // первый потенциометр (A0=GND)
#define MCP4561_ADDR_BASS 0x2F    // второй потенциометр (A0=VDD)
#define MCP4561_CMD_WRITE_WIPER0 0x00
#define MCP4561_CMD_WRITE_NV_WIPER0 0x20

#define POT_MAX_VALUE 255  // Максимальное значение потенциометра (0-255)

// --- Символы обрамления сообщений ---
#define MSG_START '['
#define MSG_END ']'

// --- Переменные для хранения текущих значений ---
int currentVolume = -1;
int currentBass = -1;

// --- Функция отправки JSON ответа с обрамлением и переводом строки ---
void sendResponse(JsonDocument& doc) {
  Serial.print(MSG_START);
  serializeJson(doc, Serial);
  Serial.print(MSG_END);
  Serial.println();  // Добавляет \n, завершая сообщение
}

// --- Универсальная функция установки значения потенциометра по адресу ---
bool setPotValue(uint8_t deviceAddr, int value) {
  // Ограничиваем значение от 0 до 255
  if (value < 0) value = 0;
  if (value > POT_MAX_VALUE) value = POT_MAX_VALUE;

  Wire.beginTransmission(deviceAddr);
  Wire.write(MCP4561_CMD_WRITE_WIPER0);
  Wire.write((uint8_t)value);
  byte error = Wire.endTransmission();

  return (error == 0);
}

bool setPotValueMemory(uint8_t deviceAddr, int value) {
  // Ограничиваем значение от 0 до 255
  if (value < 0) value = 0;
  if (value > POT_MAX_VALUE) value = POT_MAX_VALUE;

  Wire.beginTransmission(deviceAddr);
  Wire.write(MCP4561_CMD_WRITE_WIPER0);
  Wire.write((uint8_t)value);
  Wire.write(MCP4561_CMD_WRITE_NV_WIPER0);
  Wire.write((uint8_t)value);
  byte error = Wire.endTransmission();

  return (error == 0);
}

// --- Функция установки громкости (прямая запись значения 0-255) ---
void setVolume(int value) {
  bool success = setPotValue(MCP4561_ADDR_VOLUME, value);

  StaticJsonDocument<128> responseDoc;

  if (success) {
    currentVolume = value;
    responseDoc["status"] = "success";
    responseDoc["volume"] = value;
  } else {
    responseDoc["status"] = "error";
    responseDoc["message"] = "I2C communication failed (volume)";
    // Код ошибки можно добавить, но для простоты опустим
  }

  sendResponse(responseDoc);
}

// --- Функция установки уровня баса (значение 0-255) ---
void setBassLevel(int value) {
  bool success = setPotValueMemory(MCP4561_ADDR_BASS, value);

  StaticJsonDocument<128> responseDoc;

  if (success) {
    currentBass = value;
    responseDoc["status"] = "success";
    responseDoc["value"] = value;
  } else {
    responseDoc["status"] = "error";
    responseDoc["message"] = "I2C communication failed (value)";
  }

  sendResponse(responseDoc);
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
      setVolume(value);
    } else {
      StaticJsonDocument<128> errorDoc;
      errorDoc["status"] = "error";
      errorDoc["message"] = "value must be 0-255";
      errorDoc["received"] = value;
      sendResponse(errorDoc);
    }
  } else if (strcmp(command, "set_bass_level") == 0) {
    int value = doc["value"] | -1;
    if (value >= 0 && value <= POT_MAX_VALUE) {
      setBassLevel(value);
    } else {
      StaticJsonDocument<128> errorDoc;
      errorDoc["status"] = "error";
      errorDoc["message"] = "value level must be 0-255";
      errorDoc["received"] = value;
      sendResponse(errorDoc);
    }
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

  delay(10);
}