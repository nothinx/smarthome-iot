// =====================================================================
//  config.js  —  Pengaturan koneksi broker untuk website
//  Pakai broker LOKAL (Mosquitto di LAN), plain WebSocket (ws).
//  PENTING: halaman HARUS dibuka via http lokal (mis. http://localhost:8000
//  atau http://192.168.4.180:8000) — BUKAN dari GitHub Pages (https),
//  karena https tidak boleh konek ke ws:// (mixed content diblokir browser).
// =====================================================================
window.SMARTHOME_CONFIG = {
  // Broker lokal: WebSocket plain di port 9001
  protocol: "ws",        // "ws" (lokal/tanpa TLS) atau "wss" (broker cloud)
  host: "192.168.4.180", // IP broker lokal (sama dgn MQTT_HOST firmware)
  port: 9001,            // listener websockets Mosquitto
  path: "",              // Mosquitto websockets melayani di root
  username: "",          // broker lokal: kosong (anonim)
  password: "",          // broker lokal: kosong (anonim)

  // Prefix topik UNIK — WAJIB sama persis dengan TOPIC_NS di config.h
  // Ganti dengan string acak milik tim agar tak bentrok dengan orang lain.
  namespace: "r2c-sh-7dc6b0",

  // Jumlah alat di workshop (membuat pilihan 01..N)
  deviceCount: 10,

  // Nama tampilan kedua perangkat (boleh diganti)
  deviceNames: ["Perangkat 1", "Perangkat 2"],
};
