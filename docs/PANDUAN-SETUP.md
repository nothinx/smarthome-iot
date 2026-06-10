# Panduan Setup Smarthome IoT (Workshop)

Memakai **broker publik open-access** (tanpa daftar akun).
Urutan: **(1) Tentukan prefix → (2) Firmware → (3) Website → (4) Uji**.

---

## 1. Broker publik & prefix unik

Kita pakai broker publik **`broker.emqx.io`** (gratis, tanpa daftar). Ia
menyediakan port aman yang kita butuhkan:

| Klien | Host | Port | Protokol |
|---|---|---|---|
| ESP32 | `broker.emqx.io` | **8883** | MQTT over TLS |
| Website | `broker.emqx.io` | **8084** | MQTT over WebSocket Secure, path `/mqtt` |

> **PENTING — prefix unik.** Broker publik dipakai banyak orang di seluruh
> dunia dan **tanpa password**. Agar alat Anda tidak bentrok/diganggu, ganti
> nilai `TOPIC_NS` (firmware) dan `namespace` (web) dengan **string acak unik**
> milik tim Anda, mis. `r2c-sh-9x4kq2`. **Keduanya harus sama persis.**
> Siapa pun yang tidak tahu prefix Anda tidak bisa mengontrol alat.

> Untuk keandalan maksimum (mis. demo penting), Anda bisa pindah ke broker
> berakun (HiveMQ Cloud) nanti: cukup isi `host`, `username`, `password` di
> kedua config dan ganti port web ke 8884. Kode mendukung keduanya.

---

## 2. Firmware ESP32

### 2.1 Software & library (Arduino IDE)
1. Install **Arduino IDE**, lalu tambah board ESP32:
   *File → Preferences → Additional Boards URL*:
   `https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json`
   lalu *Tools → Board Manager → cari "esp32" → Install*.
2. *Tools → Manage Libraries*, install:
   - **WiFiManager** (tzapu)
   - **PubSubClient** (Nick O'Leary)
   - **ArduinoJson** (Benoit Blanchon)

### 2.2 Isi konfigurasi
Buka `firmware/smarthome/config.h` dan isi:
```c
#define DEVICE_ID 1                 // GANTI 1..10 untuk tiap alat
#define TOPIC_NS  "r2c-sh-9x4kq2"   // GANTI: string acak unik (sama dgn web)
```
> Host broker, port, pin (IN1=22, IN2=23), dan zona WIB sudah terisi benar.
> `MQTT_USER`/`MQTT_PASS` dibiarkan kosong (broker publik anonim).

### 2.3 Flash
1. Buka `firmware/smarthome/smarthome.ino` (file `config.h` ikut otomatis).
2. *Tools → Board* → **"ESP32 Dev Module"**, pilih **Port** yang benar.
3. Klik **Upload**. Buka **Serial Monitor** (115200 baud).

### 2.4 Sambungkan ke WiFi (captive portal)
1. Saat pertama nyala, ESP membuat hotspot **`Smarthome-Setup-01`** (sesuai ID).
2. Dari HP/laptop, sambung ke hotspot itu → halaman config terbuka otomatis
   (kalau tidak, buka `192.168.4.1`).
3. Pilih WiFi venue, masukkan password → **Save**. ESP restart & tersambung.
   > WiFi tersimpan; tidak perlu ulang.

**Untuk 10 alat:** ulangi 2.2–2.4, cukup ganti `DEVICE_ID` tiap unit
(01, 02, …, 10) — `TOPIC_NS` tetap sama. Tempel label angka di tiap alat.

---

## 3. Website

### 3.1 Isi konfigurasi
Buka `web/config.js` dan samakan `namespace` dengan `TOPIC_NS` firmware:
```js
host: "broker.emqx.io",
port: 8084,
namespace: "r2c-sh-9x4kq2",   // HARUS sama dengan TOPIC_NS di config.h
deviceCount: 10,
```

### 3.2 Hosting di GitHub Pages
1. Buat repo GitHub baru, upload **isi folder `web/`** (index.html, style.css,
   app.js, config.js) ke root repo.
2. Repo → **Settings → Pages** → Source: **Deploy from branch** → `main` / `root`.
3. Tunggu ~1 menit, dapat URL publik mis. `https://namauser.github.io/smarthome/`.
4. Bagikan URL itu ke peserta. Tiap peserta buka, pilih nomor alatnya.

> Alternatif cepat: drag-drop folder `web/` ke <https://app.netlify.com/drop>.

### 3.3 Uji lokal (opsional, sebelum hosting)
Dari folder `web/`:
```bash
python -m http.server 8000
```
Buka <http://localhost:8000>. (Pakai server, jangan buka file langsung,
agar koneksi WSS stabil.)

---

## 4. Uji Sistem

### 4.1 Cek di website
- Status broker pojok kanan atas → **Tersambung** (hijau).
- Pilih **Alat 01** → indikator **Online** jika ESP-01 hidup.
- Tekan tombol perangkat → relay berbunyi *klik*, badge berubah **ON/OFF**.
- Atur jadwal: aktifkan, isi jam Nyala/Mati, **Simpan**. Tutup & buka lagi web →
  jadwal tetap (tersimpan retained + di NVS ESP).

### 4.2 Uji tanpa hardware (cek alur MQTT)
Pakai **MQTT Explorer** (<http://mqtt-explorer.com>) atau klien web EMQX:
- Connect ke `broker.emqx.io`, port 8883 (TLS) — tanpa user/pass.
- Publish ke `<NS>/alat-01/relay/1/set` payload `ON` → website Alat 01 berubah ON.
- Lihat topic retained `<NS>/alat-01/relay/1/state`, `<NS>/alat-01/status`.

### 4.3 Uji jadwal cepat
Set jam **Nyala** = 1–2 menit dari sekarang, **Mati** = semenit setelahnya,
aktifkan, Simpan. Amati relay nyala lalu mati otomatis pada menit tsb.

---

## Troubleshooting

| Gejala | Penyebab & solusi |
|---|---|
| Web "Error koneksi" | Cek `host`/`port 8084`/`path /mqtt` di `config.js`. |
| Alat tak bereaksi dari web | `namespace` (web) ≠ `TOPIC_NS` (firmware). Samakan persis. |
| Alat lain ikut menyala | Prefix masih default/ketebak. Ganti `TOPIC_NS` jadi acak unik. |
| ESP tak connect MQTT (Serial `rc=-2/-4`) | WiFi venue blokir port 8883, atau broker publik sibuk. Coba hotspot HP / ulangi. |
| Relay nyala saat baru dicolok | Pastikan board active-low; firmware sudah set OFF dulu saat boot. |
| Jadwal meleset | Jam belum sinkron NTP — cek Serial ada `[ntp] ... ok`. Butuh internet. |
| Indikator Offline padahal alat hidup | Alat belum konek broker / beda nomor alat dipilih. |
| Hotspot setup tak muncul | Reset ESP; sambung manual ke `Smarthome-Setup-NN`, buka `192.168.4.1`. |
| State lama "nyangkut" di web | Pesan retained tersimpan di broker. Hapus: publish payload **kosong** (retain on) ke topik `state`/`status` terkait, atau ganti `TOPIC_NS`. |
