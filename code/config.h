#ifndef CONFIG_H
#define CONFIG_H

/* ==========================================================
   📌 CẤU HÌNH CHUNG
   ========================================================== */

// --- Chân GPIO ---
#define WIFI_STATUS_LED_PIN 2 // Chân LED hiển thị trạng thái WiFi (thường là LED trên board)
#define WIFI_RESET_BUTTON_PIN 0 // Nút nhấn để reset cấu hình WiFi (thường là nút BOOT)


/* ==========================================================
   ⚙️  CẤU HÌNH OTA (Over-the-Air Update)
   ========================================================== */

#define CURRENT_VERSION "1.0.1" // Phiên bản firmware hiện tại của thiết bị

//Mật khẩu truy cập cấu hình web
#define WEB_PASSWORD "0123456789" 

// URL trỏ tới file JSON chứa phiên bản firmware mới nhất trên GitHub
#define VERSION_URL    "https://raw.githubusercontent.com/haha123321as-alt/ESP32_UFW_OTA/main/version.json"

// URL trỏ tới file firmware (.bin) mới nhất trên GitHub
#define FIRMWARE_URL   "https://github.com/haha123321as-alt/ESP32_UFW_OTA/releases/download/ESP32_newest/SMART_HOME_nov05a.ino.bin"

// Khoảng thời gian (ms) giữa mỗi lần kiểm tra cập nhật
#define OTA_UPDATE_INTERVAL  (24*60 * 60 * 1000UL)// cập nhật sau mỗi 1 ngày nếu đc gọi hàm này vào file chính


#endif
