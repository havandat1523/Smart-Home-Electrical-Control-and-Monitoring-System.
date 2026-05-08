#include <WiFi.h>
#include <EEPROM.h>
#include <WebServer.h>
#include "config.h" // Sử dụng cấu hình tập trung

#define LED_PIN WIFI_STATUS_LED_PIN     // LED trạng thái
#define BUTTON_PIN WIFI_RESET_BUTTON_PIN // Nút Boot


WebServer server(80);

String ssid;
String password;
int wifiMode = 0; // 0: AP config, 1: connected, 2: disconnected
bool wifiCleared = false;
unsigned long buttonPressStart = 0;
bool buttonHeld = false;

//  Trạng thái đăng nhập web
bool isAuthenticated = false;

// ------------------------
// GIAO DIỆN WEB HTML
// ------------------------
const char HTML_PAGE[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta charset="UTF-8">
  <title>Cấu hình WiFi ESP32</title>
  <style>
    body { font-family: Arial; background:#f5f5f5; text-align:center; margin-top:50px; }
    h2 { color:#333; }
    form { background:white; display:inline-block; padding:20px; border-radius:10px; box-shadow:0 0 10px rgba(0,0,0,0.1); }
    input[type=text], input[type=password] {
      width: 80%%; padding: 10px; margin:10px 0; border:1px solid #ccc; border-radius:5px;
    }
    input[type=submit] {
      background:#007bff; color:white; border:none; padding:10px 20px; border-radius:5px; cursor:pointer;
    }
    input[type=submit]:hover { background:#0056b3; }
  </style>
</head>
<body>
  <h2>⚙️ Cấu hình WiFi cho ESP32</h2>
  <form action="/save" method="post">
    <input type="text" name="ssid" placeholder="Tên WiFi" required><br>
    <input type="password" name="pass" placeholder="Mật khẩu WiFi"><br>
    <input type="submit" value="Lưu & Kết nối">
  </form>
</body>
</html>
)rawliteral";

//  Giao diện đăng nhập
const char LOGIN_PAGE[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta charset="UTF-8">
  <title>Đăng nhập cấu hình ESP32</title>
  <style>
    body { font-family: Arial; background:#f5f5f5; text-align:center; margin-top:50px; }
    form { background:white; display:inline-block; padding:20px; border-radius:10px; box-shadow:0 0 10px rgba(0,0,0,0.1); }
    input[type=password] {
      width:80%%; padding:10px; margin:10px 0; border:1px solid #ccc; border-radius:5px;
    }
    input[type=submit] {
      background:#28a745; color:white; border:none; padding:10px 20px; border-radius:5px; cursor:pointer;
    }
    input[type=submit]:hover { background:#1e7e34; }
  </style>
</head>
<body>
  <h2>🔐 Nhập mật khẩu truy cập</h2>
  <form action="/auth" method="post">
    <input type="password" name="pass" placeholder="Nhập mật khẩu..." required><br>
    <input type="submit" value="Đăng nhập">
  </form>
</body>
</html>
)rawliteral";

// ------------------------
// XỬ LÝ WEB SERVER
// ------------------------
void handleRoot() {
  if (!isAuthenticated) {
    server.send(200, "text/html", LOGIN_PAGE);
    return;
  }
  server.send(200, "text/html", HTML_PAGE);
}

//  Xử lý khi người dùng nhập mật khẩu
void handleAuth() {
  String inputPass = server.arg("pass");
  if (inputPass == WEB_PASSWORD) {
    isAuthenticated = true;
    Serial.println("🔓 Xác thực thành công!");
    server.send(200, "text/html",
      "<script>alert('login complete !'); location.href='/'</script>");
  } else {
    Serial.println("Sai mật khẩu!");
    server.send(200, "text/html",
      "<script>alert('Incorrect password.!'); location.href='/'</script>");
  }
}

void handleSave() {
  String newSsid = server.arg("ssid");
  String newPass = server.arg("pass");

  Serial.println("📥 Nhận cấu hình mới:");
  Serial.println("SSID: " + newSsid);
  Serial.println("PASS: " + newPass);

  EEPROM.writeString(0, newSsid);
  EEPROM.writeString(32, newPass);
  EEPROM.commit();

  server.send(200, "text/html", "<script>alert('Success, please close the current page. ...');</script>");
  delay(2000);
  ESP.restart();
}

// ------------------------
// KHỞI TẠO WEB SERVER
// ------------------------
void startWebServer() {
  // ✅ [THÊM MỚI] Đăng ký route cho đăng nhập
  server.on("/", handleRoot);
  server.on("/auth", HTTP_POST, handleAuth);
  server.on("/save", HTTP_POST, handleSave);
  server.begin();
  Serial.println("🌐 Web server started!");
}

// ------------------------
// HÀM BẮT SỰ KIỆN WIFI
// ------------------------
void WiFiEvent(WiFiEvent_t event) {
  if (wifiCleared) return;
  switch (event) {
    case ARDUINO_EVENT_WIFI_STA_GOT_IP:
      Serial.println("✅ Connected to WiFi!");
      Serial.print("📡 IP Address: ");
      Serial.println(WiFi.localIP());
      wifiMode = 1;
      break;
    case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
      if (!wifiCleared) {
        Serial.println("⚠️ WiFi disconnected! Reconnecting...");
        wifiMode = 2;
        WiFi.disconnect();
        delay(1000);
        WiFi.begin(ssid.c_str(), password.c_str());
      }
      break;
    default: break;
  }
}

// ------------------------
// PHÁT ACCESS POINT
// ------------------------
void startAPMode() {
  WiFi.disconnect(true, true);
  delay(500);
  WiFi.mode(WIFI_AP);

  WiFi.softAPConfig(IPAddress(192,168,4,1), IPAddress(192,168,4,1), IPAddress(255,255,255,0));

  uint8_t macAddr[6];
  WiFi.softAPmacAddress(macAddr);
  String ssid_ap = "ESP32-" + String(macAddr[4], HEX) + String(macAddr[5], HEX);
  ssid_ap.toUpperCase();
  WiFi.softAP(ssid_ap.c_str());
  delay(1000);

  Serial.println("🌐 Access Point: " + ssid_ap);
  Serial.println("📶 Web server tại: http://192.168.4.1");
  wifiMode = 0;
  startWebServer();
}

// ------------------------
// XÓA WIFI TRONG EEPROM
// ------------------------
void clearWifiEEPROM() {
  Serial.println("🧹 Xóa thông tin WiFi trong EEPROM...");
  wifiCleared = true;
  WiFi.disconnect(true, true);
  WiFi.mode(WIFI_OFF);
  delay(500);

  for (int i = 0; i < 100; i++) EEPROM.write(i, 0);
  EEPROM.commit();
  Serial.println("✅ Xóa xong! Phát lại WiFi AP để cấu hình...");

  delay(1000);
  ESP.restart();
}

// ------------------------
// KIỂM TRA NÚT NHẤN
// ------------------------
void checkButton() {
  int buttonState = digitalRead(BUTTON_PIN);
  if (buttonState == LOW) {
    if (buttonPressStart == 0) buttonPressStart = millis();
    if (millis() - buttonPressStart > 5000 && !buttonHeld) {
      buttonHeld = true;
      clearWifiEEPROM();
    }
  } else {
    buttonPressStart = 0;
    buttonHeld = false;
  }
}

// ------------------------
// CẬP NHẬT LED TRẠNG THÁI
// ------------------------
void updateLED() {
  static unsigned long lastBlink = 0;
  static bool ledState = false;

  if (wifiMode == 1) digitalWrite(LED_PIN, HIGH);
  else {
    if (millis() - lastBlink >= 300) {
      lastBlink = millis();
      ledState = !ledState;
      digitalWrite(LED_PIN, ledState);
    }
  }
}

// ------------------------
// THIẾT LẬP WIFI
// ------------------------
void setupWifi() {
  pinMode(LED_PIN, OUTPUT);
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  Serial.println("\n🔍 Đọc thông tin WiFi từ EEPROM...");
  EEPROM.begin(100);
  ssid = EEPROM.readString(0);
  password = EEPROM.readString(32);

  ssid.trim(); password.trim();

  Serial.println("WiFi name: " + ssid);
  Serial.println("Password: " + password);

  if (ssid.length() > 0) {
    Serial.println("Kết nối tới WiFi đã lưu...");
    WiFi.mode(WIFI_STA);
    WiFi.onEvent(WiFiEvent);
    WiFi.begin(ssid.c_str(), password.c_str());

    unsigned long startAttempt = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - startAttempt < 10000) {
      Serial.print(".");
      delay(500);
      updateLED();
    }

    if (WiFi.status() == WL_CONNECTED) {
      Serial.println("\n✅ Kết nối thành công!");
      Serial.print("IP Address: ");
      Serial.println(WiFi.localIP());
      wifiMode = 1;
      startWebServer(); // vẫn chạy webserver trong mạng LAN
    } else {
      Serial.println("\n❌ Không kết nối được, bật chế độ AP để cấu hình!");
      startAPMode();
    }
  } else {
    Serial.println("Không có WiFi lưu trữ, bật AP để cấu hình!");
    startAPMode();
  }
}

// ------------------------
// GỌI TRONG LOOP()
// ------------------------
void wifiLoop() {
  updateLED();
  checkButton();
  server.handleClient();
}

// ------------------------
// GỌI TRONG setup()
// ------------------------
void wifiConfigSetup() {
  setupWifi();
}
