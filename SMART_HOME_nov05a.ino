#include "arduino_secrets.h"
#include "config.h"
#include "wifiConfig.h"
#include "OTAUpdate.h"
#include "thingProperties.h"

// =================== CẤU HÌNH CHÂN ===================
const int buttonPins[8] = { 23, 22, 21, 19, 18, 5, 4, 15 };  // Nút nhấn
const int Relays[8] = { 13, 32, 12, 33, 14, 25, 27, 26 };    // điều khiển Relay

// Trạng thái chống dội
bool buttonState[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };
bool lastButtonState[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };
unsigned long lastTime[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };
const unsigned long timeDelay = 50;  // ms

// =====================================================
void setup() {
  Serial.begin(115200);
  delay(1500);
  // Cấu hình LED & nút nhấn
  for (int i = 0; i < 8; i++) {
    pinMode(Relays[i], OUTPUT);
    digitalWrite(Relays[i], LOW);
    pinMode(buttonPins[i], INPUT_PULLDOWN);
  }

  // 1️⃣ Khởi động WiFi Config
  wifiConfigSetup();

  // 2️⃣ Nếu WiFi đã sẵn sàng, khởi động Cloud & OTA
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("✅ WiFi OK, Start Cloud & OTA...");

    initProperties();
    ArduinoCloud.begin(ArduinoIoTPreferredConnection);

    setDebugMessageLevel(2);
    ArduinoCloud.printDebugInfo();

    initialOtaCheck();  // Kiểm tra OTA ngay khi có WiFi
  } else {
    Serial.println("⚠️ Mode AP. Skip Cloud & OTA.");
    Serial.println("Please connect ESP32-XXXX and access http://192.168.4.1");
  }
}

// =====================================================
void loop() {
  wifiLoop();  // Duy trì webserver cấu hình WiFi

  if (WiFi.status() == WL_CONNECTED) {
    ArduinoCloud.update();
    // otaLoop(); // Cập nhật OTA định kỳ
  }

  handleButtons();  // Đọc nút nhấn
}

// =====================================================
void handleButtons() {
  for (int i = 0; i < 8; i++) {
    int reading = digitalRead(buttonPins[i]);

    if (reading != lastButtonState[i]) {
      lastTime[i] = millis();
    }

    if ((millis() - lastTime[i]) > timeDelay) {
      if (reading != buttonState[i]) {
        buttonState[i] = reading;

        if (reading == HIGH) {  // Nút được nhấn
          bool newState = !digitalRead(Relays[i]);
          digitalWrite(Relays[i], newState);

          // Cập nhật biến Cloud tương ứng
          switch (i) {
            case 0: led1 = newState; break;
            case 1: led2 = newState; break;
            case 2: led3 = newState; break;
            case 3: led4 = newState; break;
            case 4: led5 = newState; break;
          }
          Serial.print("Relay ");
          Serial.print(i + 1);
          Serial.println(newState ? " bật" : " tắt");
        }
      }
    }
    lastButtonState[i] = reading;
  }
}

// =====================================================
void onLed1Change() {
  digitalWrite(Relays[0], led1);
  Serial.println(led1 ? "Relay 1 bật" : "Relay 1 tắt");
}
void onLed2Change() {
  digitalWrite(Relays[1], led2);
  Serial.println(led2 ? "Relay 2 bật" : "Relay 2 tắt");
}
void onLed3Change() {
  digitalWrite(Relays[2], led3);
  Serial.println(led3 ? "Relay 3 bật" : "Relay 3 tắt");
}
void onLed4Change() {
  digitalWrite(Relays[3], led4);
  Serial.println(led4 ? "Relay 4 bật" : "Relay 4 tắt");
}
void onLed5Change() {
  digitalWrite(Relays[4], led5);
  Serial.println(led5 ? "Relay 5 bật" : "Relay 5 tắt");
}