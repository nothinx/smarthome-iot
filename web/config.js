// =====================================================================
//  config.js  —  Pengaturan koneksi broker untuk website
//  Pakai broker PUBLIK (open access) — tidak perlu daftar akun.
// =====================================================================
window.SMARTHOME_CONFIG = {
  // Broker publik EMQX (MQTTS 8883 utk ESP, WSS 8084 utk web)
  host: "broker.emqx.io",
  port: 8084,            // WSS (WebSocket Secure)
  path: "/mqtt",
  username: "",          // broker publik: kosong (anonim)
  password: "",          // broker publik: kosong (anonim)

  // Prefix topik UNIK — WAJIB sama persis dengan TOPIC_NS di config.h
  // Ganti dengan string acak milik tim agar tak bentrok dengan orang lain.
  namespace: "r2c-sh-ganti7acak",

  // Jumlah alat di workshop (membuat pilihan 01..N)
  deviceCount: 10,

  // Nama tampilan kedua perangkat (boleh diganti)
  deviceNames: ["Perangkat 1", "Perangkat 2"],
};
