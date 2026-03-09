#include <Wire.h>
#include <ArduinoJson.h>

// --- Адреса и константы MCP4561 ---
#define MCP4561_ADDR 0x2E
#define MCP4561_CMD_WRITE_WIPER0 0x00

#define POT_MAX_VALUE 255 // Максимальное значение потенциометра (0-255)

// --- Символы обрамления сообщений ---
#define MSG_START '['
#define MSG_END ']'

// --- Переменные ---
int currentVolume = -1;

// --- Функция отправки JSON ответа с обрамлением и переводом строки ---
void sendResponse(JsonDocument &doc) {
  Serial.print(MSG_START);
  serializeJson(doc, Serial);
  Serial.print(MSG_END);
  Serial.println(); // Добавляет \n, завершая сообщение
}

// --- Функция установки громкости (прямая запись значения 0-255) ---
void setVolume(int volumeValue) {
  // Ограничиваем значение от 0 до 255
  if (volumeValue < 0) volumeValue = 0;
  if (volumeValue > POT_MAX_VALUE) volumeValue = POT_MAX_VALUE;
  
  Wire.beginTransmission(MCP4561_ADDR);
  Wire.write(MCP4561_CMD_WRITE_WIPER0);
  Wire.write((uint8_t)volumeValue);
  byte error = Wire.endTransmission();

  StaticJsonDocument<128> responseDoc;
  
  if (error == 0) {
    currentVolume = volumeValue;
    responseDoc["status"] = "success";
    responseDoc["volume"] = volumeValue;
  } else {
    responseDoc["status"] = "error";
    responseDoc["message"] = "I2C communication failed";
    responseDoc["error_code"] = error;
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
    int vol = doc["volume"] | -1;
    if (vol >= 0 && vol <= POT_MAX_VALUE) {
      setVolume(vol);
    } else {
      StaticJsonDocument<128> errorDoc;
      errorDoc["status"] = "error";
      errorDoc["message"] = "Volume must be 0-255";
      errorDoc["received"] = vol;
      sendResponse(errorDoc);
    }
  }
  else if (strcmp(command, "get_volume") == 0) {
    StaticJsonDocument<128> responseDoc;
    responseDoc["status"] = "success";
    responseDoc["command"] = "get_volume";
    responseDoc["volume"] = currentVolume;
    sendResponse(responseDoc);
  }
  else if (strcmp(command, "ping") == 0) {
    StaticJsonDocument<128> responseDoc;
    responseDoc["status"] = "success";
    responseDoc["command"] = "pong";
    responseDoc["device"] = "MCP4561_Volume_Adapter";
    responseDoc["volume_range"] = "0-255";
    sendResponse(responseDoc);
  }
  else {
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
  
  sendResponse(welcomeDoc);
  delay(100);
  setVolume(1);
}

// --- Loop с чтением строк до '\n' ---
void loop() {
  if (Serial.available() > 0) {
    String input = Serial.readStringUntil('\n'); // читаем до \n, сам \n не включается
    input.trim(); // удаляем пробельные символы по краям (включая \r, если есть)
    
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