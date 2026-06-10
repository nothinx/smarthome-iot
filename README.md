# Smarthome IoT — Workshop (10 Alat)

Sistem smarthome berbasis **ESP32 + 2 relay active-low**, dikontrol lewat
**website** via **MQTT (broker lokal Mosquitto)**. Mendukung kontrol manual
ON/OFF dan **jadwal jam harian** per relay. Untuk **workshop 10 alat identik**.

```
[Website (http lokal)] --ws:9001--> [Mosquitto LAN] <--tcp:1883-- [ESP32 x10]
   pilih alat 01..10                192.168.4.180                  relay + jadwal
```

> Semua perangkat (ESP, broker, browser) harus di **WiFi/LAN yang sama**.
> Halaman dibuka via **http lokal**, bukan https (lihat Panduan Setup).

## Fitur
- Kontrol manual ON/OFF dua perangkat per alat.
- Jadwal jam harian per perangkat (jalan di ESP via NTP, tahan reboot).
- 10 alat identik dibedakan `DEVICE_ID` (01–10), topic MQTT ber-namespace.
- Boot aman (relay tidak nyala sendiri saat dicolok), state tersimpan di NVS.
- Status online/offline real-time (MQTT Last Will).
- Website statis tema gelap, responsif — tanpa backend.

## Struktur
```
smarthome-iot/
├── firmware/smarthome/     # sketch Arduino ESP32
│   ├── smarthome.ino
│   └── config.h            # << isi DEVICE_ID + kredensial broker
├── web/                    # website (GitHub Pages)
│   ├── index.html
│   ├── style.css
│   ├── app.js
│   └── config.js           # << isi host + kredensial broker
├── docs/
│   ├── PANDUAN-SETUP.md     # langkah lengkap: broker, flash, hosting, uji
│   └── superpowers/specs/   # dokumen desain
└── README.md
```

## Mulai cepat
1. **Broker**: jalankan Mosquitto lokal (listener `1883` plain + `9001`
   websockets). Pastikan IP-nya sesuai `MQTT_HOST` & `host` di config.
2. **Firmware**: isi `firmware/smarthome/config.h`, ganti `DEVICE_ID` per alat, flash.
3. **WiFi**: kredensial AP sudah hardcoded di `config.h` (`R2C`/`juarajuara`),
   ESP langsung connect — pastikan AP & broker di LAN yang sama.
4. **Web**: jalankan folder `web/` via **http lokal** (`python -m http.server`),
   buka dari perangkat di LAN yang sama.

➡️ Langkah detail di **[docs/PANDUAN-SETUP.md](docs/PANDUAN-SETUP.md)**.

## Topic MQTT
Awalan per alat: `<NS>/alat-NN/` (NS = prefix unik Anda)

| Topic | Arah | Payload |
|---|---|---|
| `relay/{1,2}/set` | web → ESP | `ON` / `OFF` |
| `relay/{1,2}/state` | ESP → web | `ON` / `OFF` (retained) |
| `schedule/{1,2}/set` | web → ESP | `{"enabled":true,"on":"18:00","off":"22:00"}` |
| `schedule/{1,2}/state` | ESP → web | JSON (retained) |
| `status` | ESP → web | `online` / `offline` (retained, LWT) |
