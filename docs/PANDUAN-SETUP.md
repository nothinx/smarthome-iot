# Panduan Setup Smarthome IoT (Workshop)

Memakai **broker lokal (Mosquitto)** di LAN — semua di jaringan yang sama.
Urutan: **(1) Broker lokal → (2) Firmware → (3) Website → (4) Uji**.

> **SYARAT JARINGAN.** ESP, komputer broker, dan perangkat yang membuka website
> **harus terhubung ke WiFi/LAN yang sama**. Broker lokal tidak bisa diakses
> dari internet luar.

---

## 1. Broker lokal (Mosquitto)

Komputer teman Anda menjalankan broker. Pastikan dua listener aktif:

| Klien | Host | Port | Protokol |
|---|---|---|---|
| ESP32 | `192.168.4.180` | **1883** | MQTT plain (TCP) |
| Website | `192.168.4.180` | **9001** | MQTT over WebSocket (ws), root path |

Contoh isi `mosquitto.conf` yang dibutuhkan:
```conf
listener 1883
protocol mqtt

listener 9001
protocol websockets

allow_anonymous true
```
Jalankan: `mosquitto -c mosquitto.conf -v`. Pastikan firewall mengizinkan port
1883 & 9001.

> **Ganti IP `192.168.4.180`** bila IP komputer broker berbeda. Nilai ini harus
> sama di `MQTT_HOST` (firmware `config.h`) dan `host` (web `config.js`).
> Cek IP: `ipconfig` (Windows) / `ip a` (Linux) / `ifconfig` (Mac).

> Prefix `TOPIC_NS`/`namespace` (mis. `r2c-sh-7dc6b0`) boleh tetap — di broker
> lokal tidak wajib unik, tapi biarkan sama di kedua config.

---

## 2. Firmware ESP32

### 2.1 Software & library (Arduino IDE)
1. Install **Arduino IDE**, lalu tambah board ESP32:
   *File → Preferences → Additional Boards URL*:
   `https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json`
   lalu *Tools → Board Manager → cari "esp32" → Install*.
2. *Tools → Manage Libraries*, install:
   - **PubSubClient** (Nick O'Leary)
   - **ArduinoJson** (Benoit Blanchon)

### 2.2 Isi konfigurasi
Buka `firmware/smarthome/config.h`. Yang perlu dicek:
```c
#define DEVICE_ID 1               // GANTI 1..10 untuk tiap alat
#define MQTT_HOST "192.168.4.180" // IP broker lokal (sesuaikan bila beda)
#define WIFI_SSID "R2C"           // SSID AP Tenda
#define WIFI_PASS "juarajuara"    // password AP
```
> Port (1883), pin (IN1=22, IN2=23), zona WIB, dan `TOPIC_NS` sudah terisi.
> WiFi sudah **hardcoded** — tidak ada captive portal lagi, langsung connect.

### 2.3 Flash
1. Buka `firmware/smarthome/smarthome.ino` (file `config.h` ikut otomatis).
2. *Tools → Board* → **"ESP32 Dev Module"**, pilih **Port** yang benar.
3. Klik **Upload**. Buka **Serial Monitor** (115200 baud).

### 2.4 WiFi otomatis
Tidak ada langkah manual. Saat nyala, ESP langsung menyambung ke AP `R2C`.
Di Serial Monitor akan muncul:
```
[wifi] menyambung ke R2C... 
[wifi] tersambung: 192.168.4.xxx
[mqtt] menghubungkan... tersambung
```

**Untuk 10 alat:** ulangi 2.2–2.3, cukup ganti `DEVICE_ID` tiap unit
(01, 02, …, 10) — sisanya sama. Tempel label angka di tiap alat.

---

## 3. Website (dijalankan via HTTP LOKAL)

> ⚠️ **Jangan pakai URL GitHub Pages (https).** Broker lokal memakai `ws://`
> (tanpa TLS), dan halaman https **dilarang** browser konek ke `ws://`
> (mixed content). Jadi website harus dibuka via **http lokal**.

### 3.1 Isi konfigurasi
Buka `web/config.js`, pastikan menunjuk broker lokal:
```js
protocol: "ws",
host: "192.168.4.180",   // sama dengan MQTT_HOST firmware
port: 9001,              // listener websockets Mosquitto
path: "",
```

### 3.2 Jalankan via HTTP lokal
Dari folder `web/`, di komputer yang terhubung AP `R2C`:
```bash
python -m http.server 8000
```
- Buka di komputer itu: <http://localhost:8000>
- Dari HP/laptop peserta lain di WiFi sama: `http://<IP-komputer>:8000`
  (mis. `http://192.168.4.180:8000` bila dijalankan di komputer broker).

> Praktis: jalankan server web ini di **komputer broker** sekalian, supaya
> semua peserta cukup buka `http://192.168.4.180:8000`.

> Library `mqtt.js` sudah **ditanam lokal** (`web/mqtt.min.js`) — website
> berjalan **tanpa internet**. (Sebelumnya diambil dari CDN; di AP lokal tanpa
> internet itu gagal dimuat & bikin web stuck "Menyambung…".)

### 3.3 Opsi: file mandiri (1 file, offline)
`web/smarthome-standalone.html` berisi SEMUA (HTML+CSS+JS+library) dalam satu
file. Cocok dibagikan agar tiap peserta jalan sendiri:
- **Laptop:** kirim file itu, dobel-klik → langsung jalan (tanpa server).
- Tetap wajib: perangkat tersambung **WiFi `R2C`** & broker punya **listener 9001**.
- Bila `config.js` diubah (mis. IP broker), bangun ulang: `python build-standalone.py`.

> Di **HP** membuka file lokal lebih ribet — untuk HP tetap paling enak host
> sekali di komputer broker (cara 3.2).

---

## 4. Uji Sistem

### 4.1 Cek di website
- Status broker pojok kanan atas → **Tersambung** (hijau).
- Pilih **Alat 01** → indikator **Online** jika ESP-01 hidup.
- Tekan tombol perangkat → relay berbunyi *klik*, badge berubah **ON/OFF**.
- Refresh halaman / pindah alat → status terkini langsung muncul (retained).

### 4.2 Uji tanpa hardware (cek alur MQTT)
Pakai **MQTT Explorer** (<http://mqtt-explorer.com>):
- Connect ke `192.168.4.180`, port 1883 (plain) — tanpa user/pass.
- Publish ke `<NS>/alat-01/relay/1/set` payload `ON` → website Alat 01 berubah ON.
- Lihat topic retained `<NS>/alat-01/relay/1/state`, `<NS>/alat-01/status`.

---

## Troubleshooting

| Gejala | Penyebab & solusi |
|---|---|
| Web "Error koneksi" | Buka via **http lokal**, bukan https GitHub Pages (ws diblokir di https). Cek `host`/`port 9001`/`protocol: "ws"`. Pastikan broker & listener 9001 jalan. |
| Web di laptop lain tak konek | Laptop tidak di WiFi `R2C` yang sama, atau firewall komputer broker blokir port 9001. |
| Alat tak bereaksi dari web | `namespace` (web) ≠ `TOPIC_NS` (firmware). Samakan persis. |
| ESP tak connect MQTT (Serial `rc=-2`) | IP `MQTT_HOST` salah, broker belum jalan, atau listener 1883 belum aktif. Cek IP komputer broker. |
| ESP tak connect WiFi | SSID/password `R2C`/`juarajuara` salah, atau AP belum nyala. |
| Relay nyala saat baru dicolok | Pastikan board active-low; firmware sudah set OFF dulu saat boot. |
| Indikator Offline padahal alat hidup | Alat belum konek broker / beda nomor alat dipilih. |
| State lama "nyangkut" di web | Pesan retained tersimpan di broker. Hapus: publish payload **kosong** (retain on) ke topik `state`/`status` terkait. |
