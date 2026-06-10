# Desain: Smarthome IoT (Workshop 10 Alat)

**Tanggal:** 2026-06-10
**Status:** Disetujui untuk implementasi

> **Diperbarui 2026-06-11 (sebagian disuperseksi).** Implementasi final beralih
> ke **broker lokal Mosquitto** (LAN, plain `1883` + ws `9001`) dan **WiFi
> hardcoded** (AP `R2C`), bukan broker cloud EMQX + WiFiManager seperti tertulis
> di bawah. Acuan terkini: `docs/PANDUAN-SETUP.md`, `firmware/.../config.h`,
> `web/config.js`. Catatan: NTP butuh internet pada AP tsb; tanpa internet,
> fitur jadwal tidak berfungsi (kontrol manual tetap jalan).

## Tujuan

Sistem smarthome untuk **workshop berisi 10 alat identik**. Tiap alat = ESP32
DevKit v1 dengan 2 relay active-low (IN1=GPIO22, IN2=GPIO23). Dikontrol dari
website yang sama lewat MQTT (broker cloud). Fitur: kontrol manual ON/OFF dan
jadwal jam harian per relay. Dibuat menarik karena untuk pameran/workshop.

## Keputusan Arsitektur

| Topik | Keputusan | Alasan |
|---|---|---|
| Lokasi logika jadwal | Di ESP32 | Tetap jalan walau website ditutup |
| Sumber waktu | NTP (WIB, UTC+7) | Akurat, gratis, bawaan ESP32 |
| Broker | `broker.emqx.io` (publik, open-access) | Tanpa daftar; sediakan MQTTS + WSS tanpa sertifikat |
| Prefix topic unik | `TOPIC_NS` = `namespace` | Broker publik tanpa auth → cegah bentrok/gangguan |
| Port ESP (MQTTS) | 8883 | Port TLS EMQX publik |
| Port web (WSS) | 8084, path `/mqtt` | WebSocket Secure EMQX publik |
| Hosting web | GitHub Pages (statis) | Satu URL, peserta buka di HP/laptop sendiri |
| Identitas alat | `#define DEVICE_ID` (01–10) | ID terbaca (`alat-03`), tanpa layar |
| WiFi | WiFiManager captive portal | SSID/password venue dimasukkan tanpa flash ulang |
| TLS ESP | `setInsecure()` | Tetap terenkripsi, hindari error koneksi saat workshop |

## Diagram

```
   [Website GitHub Pages]                 [ESP32 #01 .. #10]
   HTML/CSS/JS + MQTT.js                   WiFi + MQTT + NTP
   - pilih nomor alat (01–10)              - 2 relay active-low (22, 23)
   - tombol ON/OFF manual                  - jadwal jam jalan di ESP
   - editor jadwal jam                     - simpan state & jadwal di NVS
          │  WSS :8084                              │  MQTTS :8883
          └──────────────►  broker.emqx.io  ◄────────┘
                            (broker publik, prefix unik <NS>)
```

## Skema Topic MQTT

Awalan per alat: `<NS>/alat-NN/` (NS = prefix unik, sama di firmware & web)

| Topic | Arah | Payload | Sifat |
|---|---|---|---|
| `relay/1/set`, `relay/2/set` | web → ESP | `ON` / `OFF` | command, QoS1 |
| `relay/1/state`, `relay/2/state` | ESP → web | `ON` / `OFF` | retained |
| `schedule/1/set`, `schedule/2/set` | web → ESP | JSON `{enabled,on,off}` | command, QoS1 |
| `schedule/1/state`, `schedule/2/state` | ESP → web | JSON | retained |
| `status` | ESP → web | `online` / `offline` | retained (LWT) |

State & status **retained** → website langsung tampilkan kondisi terkini saat
memilih alat. Payload jadwal: `{"enabled":true,"on":"18:00","off":"22:00"}`.

## Firmware ESP32

Firmware identik untuk 10 alat; hanya `#define DEVICE_ID n` yang diganti.

Poin penting:
- **Boot aman**: `digitalWrite(pin, HIGH)` sebelum `pinMode(pin, OUTPUT)` agar
  relay active-low tidak "klik nyala" saat dicolok. State terakhir dipulihkan
  dari NVS.
- **Urutan**: WiFi (via WiFiManager) → NTP sync → MQTT connect (dengan LWT).
- **Jadwal**: dievaluasi sekali tiap menit. Saat `HH:MM` == jam nyala → ON;
  == jam mati → OFF. Manual override berlaku sampai transisi jadwal berikutnya.
- **Persistensi NVS** (Preferences): state relay + jadwal tahan reboot/mati listrik.

Library: WiFiManager, PubSubClient, WiFiClientSecure, ArduinoJson, Preferences,
time.h.

## Website (GitHub Pages)

- Halaman statis tema gelap modern, kartu per perangkat, toggle beranimasi.
- Pemilih alat 01–10 + indikator online/offline (titik hijau/abu).
- Per perangkat: tombol ON/OFF besar + editor jadwal (toggle aktif, jam nyala,
  jam mati, tombol Simpan).
- MQTT.js over WSS ke HiveMQ. Tanpa backend.
- Responsif (HP & proyektor).

## Semantik Jadwal (YAGNI)

Per relay: satu jadwal harian `{enabled, on:"HH:MM", off:"HH:MM"}`, berulang
tiap hari. Manual ON/OFF dapat override kapan saja. Dapat dikembangkan ke banyak
slot/per-hari di kemudian hari.

## Lingkup

**Termasuk**: firmware, website, dok setup HiveMQ, panduan workshop.
**Tidak termasuk**: database histori, login, sensor, app mobile native.

## Pengujian

- Tanpa hardware: publish manual ke topic `set`, cek `state` retained.
- Jadwal: uji dengan jam dipercepat / set on=menit berikutnya.
- Web: cek koneksi WSS, ganti alat, refresh → state pulih dari retained.
