#ifndef OTA_UPDATE_H
#define OTA_UPDATE_H

#include <WiFi.h>
#include <HTTPClient.h>
#include <Update.h>
#include <WiFiClientSecure.h>
#include "config.h" // Sử dụng cấu hình tập trung
unsigned long lastUpdateCheck = 0;
/* ==========================================================
   ⚙️  CẤU HÌNH
   ========================================================== */
String fetchLatestVersion();
void downloadAndApplyFirmware();
void checkForFirmwareUpdate();

/* ==========================================================
   📡 LẤY PHIÊN BẢN MỚI NHẤT TỪ GITHUB
   ========================================================== */
String fetchLatestVersion() {
  HTTPClient http;
  http.begin(VERSION_URL);
  int httpCode = http.GET();

  if (httpCode == HTTP_CODE_OK) {
    String latestVersion = http.getString();
    latestVersion.trim();
    http.end();
    return latestVersion;
  } else {
    Serial.printf("⚠️ Không thể lấy version. HTTP code: %d\n", httpCode);
    http.end();
    return "";
  }
}

/* ==========================================================
   💾 GHI FIRMWARE VÀO FLASH
   ========================================================== */
bool startOTAUpdate(WiFiClient* client, int contentLength) {
  Serial.println("🚀 Bắt đầu cập nhật firmware...");
  if (!Update.begin(contentLength)) {
    Serial.printf("❌ Update.begin() thất bại: %s\n", Update.errorString());
    return false;
  }

  size_t written = 0;
  int lastProgress = -1;
  unsigned long lastDataTime = millis();
  const unsigned long timeoutDuration = 120000; // 2 phút timeout

  while (written < contentLength) {
    if (client->available()) {
      uint8_t buffer[256];
      size_t len = client->read(buffer, sizeof(buffer));
      if (len > 0) {
        Update.write(buffer, len);
        written += len;
        lastDataTime = millis();

        int progress = (written * 100) / contentLength;
        if (progress != lastProgress) {
          Serial.printf("📦 Tiến trình: %d%%\n", progress);
          lastProgress = progress;
        }
      }
    }

    if (millis() - lastDataTime > timeoutDuration) {
      Serial.println("⏰ Timeout khi tải dữ liệu!");
      Update.abort();
      return false;
    }

    yield();
  }

  if (!Update.end()) {
    Serial.printf("❌ Update.end() thất bại: %s\n", Update.errorString());
    return false;
  }

  Serial.println("✅ Ghi firmware hoàn tất!");
  return true;
}

/* ==========================================================
   🌍 TẢI VÀ ÁP DỤNG FIRMWARE MỚI
   ========================================================== */
void downloadAndApplyFirmware() {
  WiFiClientSecure client;
  client.setInsecure(); // ⚠️ demo thôi, có thể thêm CA sau

  HTTPClient http;
  http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
  http.begin(client, FIRMWARE_URL);

  int httpCode = http.GET();
  if (httpCode != HTTP_CODE_OK) {
    Serial.printf("❌ Lỗi tải firmware. HTTP code: %d\n", httpCode);
    http.end();
    return;
  }

  int contentLength = http.getSize();
  if (contentLength <= 0) {
    Serial.println("⚠️ Dung lượng firmware không hợp lệ!");
    http.end();
    return;
  }

  WiFiClient* stream = http.getStreamPtr();
  if (startOTAUpdate(stream, contentLength)) {
    Serial.println("🔁 Khởi động lại để áp dụng firmware mới...");
    delay(2000);
    ESP.restart();
  } else {
    Serial.println("❌ Cập nhật thất bại!");
  }
  http.end();
}

/* ==========================================================
   🧠 KIỂM TRA VÀ CẬP NHẬT OTA
   ========================================================== */
void checkForFirmwareUpdate() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("⚠️ WiFi chưa sẵn sàng, bỏ qua kiểm tra cập nhật.");
    return;
  }

  Serial.println("🔍 Đang kiểm tra firmware mới...");
  String latestVersion = fetchLatestVersion();

  if (latestVersion.isEmpty()) {
    Serial.println("⚠️ Không thể lấy thông tin phiên bản online!");
    return;
  }

  Serial.println("🔸 Phiên bản hiện tại: " + String(CURRENT_VERSION));
  Serial.println("🔹 Phiên bản mới nhất: " + latestVersion);

  if (latestVersion != CURRENT_VERSION) {
    Serial.println("✨ Có bản cập nhật mới! Bắt đầu tải...");
    downloadAndApplyFirmware();
  } else {
    Serial.println("✅ Firmware đang ở phiên bản mới nhất.");
  }
}

/* ==========================================================
   🚀 KHỞI TẠO OTA (GỌI TRONG setup())
   ========================================================== */
void initialOtaCheck() {
  // Chờ một chút để đảm bảo kết nối mạng ổn định
  delay(2000);
  checkForFirmwareUpdate();
}

/* ==========================================================
   🔄 GỌI TRONG loop()
   ========================================================== */
void otaLoop() {
  if (WiFi.status() == WL_CONNECTED) {
    if (millis() - lastUpdateCheck > OTA_UPDATE_INTERVAL) {
      lastUpdateCheck = millis();
      checkForFirmwareUpdate();
    }
  }
}

#endif