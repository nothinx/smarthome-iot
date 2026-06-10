// =====================================================================
//  config.h  —  Pengaturan per-alat & broker
//  Ganti nilai di bawah lalu flash. Hanya DEVICE_ID yang beda tiap unit.
// =====================================================================
#ifndef CONFIG_H
#define CONFIG_H

// --- Identitas alat: GANTI angka ini 1..10 untuk tiap unit ------------
#define DEVICE_ID 1            // -> topic "<NS>/alat-01/..."

// --- Prefix topic UNIK (WAJIB diganti) -------------------------------
// Broker publik dipakai banyak orang. Ganti dengan string acak unik milik
// tim Anda agar alat tidak bentrok/diganggu. SAMAKAN dengan web/config.js.
#define TOPIC_NS "r2c-sh-7dc6b0"

// --- Broker MQTT LOKAL (Mosquitto di LAN, plain/tanpa TLS) ------------
// ESP, broker, dan perangkat pengontrol HARUS di jaringan WiFi yang sama.
#define MQTT_HOST "192.168.4.180"  // IP broker lokal
#define MQTT_PORT 1883             // plain MQTT (web socket pakai 9001)
#define MQTT_USER ""               // broker lokal anonim: kosongkan
#define MQTT_PASS ""               // broker lokal anonim: kosongkan

// --- Pin relay (active-low: LOW = nyala) -----------------------------
#define RELAY1_PIN 22          // IN1
#define RELAY2_PIN 23          // IN2

// --- Waktu (NTP) -----------------------------------------------------
#define NTP_SERVER "pool.ntp.org"
#define TZ_OFFSET_SEC (7 * 3600)   // WIB = UTC+7
#define DST_OFFSET_SEC 0

// --- WiFi (AP lokal Tenda, kredensial tetap utk semua alat) ----------
#define WIFI_SSID "R2C"
#define WIFI_PASS "juarajuara"

#endif // CONFIG_H
