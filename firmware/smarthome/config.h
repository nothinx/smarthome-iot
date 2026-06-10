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

// --- Broker MQTT publik (open access, tanpa daftar) ------------------
// broker.emqx.io menyediakan MQTTS (ESP) + WSS (web) tanpa sertifikat.
#define MQTT_HOST "broker.emqx.io"
#define MQTT_PORT 8883         // MQTTS/TLS untuk ESP (web pakai 8084 WSS)
#define MQTT_USER ""           // broker publik: kosongkan (anonim)
#define MQTT_PASS ""           // broker publik: kosongkan (anonim)

// --- Pin relay (active-low: LOW = nyala) -----------------------------
#define RELAY1_PIN 22          // IN1
#define RELAY2_PIN 23          // IN2

// --- Waktu (NTP) -----------------------------------------------------
#define NTP_SERVER "pool.ntp.org"
#define TZ_OFFSET_SEC (7 * 3600)   // WIB = UTC+7
#define DST_OFFSET_SEC 0

// --- Nama portal konfigurasi WiFi (captive portal) -------------------
#define WIFI_PORTAL_PREFIX "Smarthome-Setup-"   // -> "Smarthome-Setup-01"

#endif // CONFIG_H
