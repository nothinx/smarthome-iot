// =====================================================================
//  Smarthome IoT — Firmware ESP32 DevKit v1
//  2 relay active-low + MQTT (broker publik) + jadwal jam (NTP) + NVS
//
//  Firmware ini IDENTIK untuk 10 alat. Yang diganti tiap unit hanya
//  DEVICE_ID di config.h.
//
//  Library (Library Manager):
//    - WiFiManager        (tzapu)
//    - PubSubClient       (Nick O'Leary)
//    - ArduinoJson        (Benoit Blanchon)
//  WiFiClientSecure, Preferences, time.h = bawaan core ESP32.
// =====================================================================
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <WiFiManager.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <Preferences.h>
#include <time.h>
#include "config.h"

// ---------------------------------------------------------------------
//  Global
// ---------------------------------------------------------------------
WiFiClientSecure netClient;
PubSubClient     mqtt(netClient);
Preferences      prefs;

char deviceTag[12];        // "alat-01"
char baseTopic[64];        // "<NS>/alat-01"

const uint8_t RELAY_PIN[2] = { RELAY1_PIN, RELAY2_PIN };
bool relayState[2] = { false, false };

struct Schedule {
  bool enabled;
  int  onMin;              // menit-dalam-hari, -1 = tak diset
  int  offMin;
};
Schedule sched[2] = { { false, -1, -1 }, { false, -1, -1 } };

int lastEvalMinute = -1;   // agar jadwal dievaluasi sekali per menit

// ---------------------------------------------------------------------
//  Util waktu
// ---------------------------------------------------------------------
int hhmmToMinutes(const char* s) {     // "18:30" -> 1110 ; invalid -> -1
  if (!s || strlen(s) < 4) return -1;
  int h = atoi(s);
  const char* colon = strchr(s, ':');
  if (!colon) return -1;
  int m = atoi(colon + 1);
  if (h < 0 || h > 23 || m < 0 || m > 59) return -1;
  return h * 60 + m;
}

void minutesToHHMM(int total, char* out) {  // -1 -> ""
  if (total < 0) { out[0] = '\0'; return; }
  sprintf(out, "%02d:%02d", total / 60, total % 60);
}

// ---------------------------------------------------------------------
//  Persistensi (NVS)
// ---------------------------------------------------------------------
void loadState() {
  prefs.begin("smarthome", true);
  relayState[0] = prefs.getBool("r0", false);
  relayState[1] = prefs.getBool("r1", false);
  sched[0].enabled = prefs.getBool("s0en", false);
  sched[0].onMin   = prefs.getInt ("s0on", -1);
  sched[0].offMin  = prefs.getInt ("s0off", -1);
  sched[1].enabled = prefs.getBool("s1en", false);
  sched[1].onMin   = prefs.getInt ("s1on", -1);
  sched[1].offMin  = prefs.getInt ("s1off", -1);
  prefs.end();
}

void saveRelay(int i) {
  prefs.begin("smarthome", false);
  prefs.putBool(i == 0 ? "r0" : "r1", relayState[i]);
  prefs.end();
}

void saveSchedule(int i) {
  prefs.begin("smarthome", false);
  if (i == 0) {
    prefs.putBool("s0en", sched[0].enabled);
    prefs.putInt ("s0on", sched[0].onMin);
    prefs.putInt ("s0off", sched[0].offMin);
  } else {
    prefs.putBool("s1en", sched[1].enabled);
    prefs.putInt ("s1on", sched[1].onMin);
    prefs.putInt ("s1off", sched[1].offMin);
  }
  prefs.end();
}

// ---------------------------------------------------------------------
//  Relay  (active-low: LOW = nyala)
// ---------------------------------------------------------------------
void applyRelay(int i) {
  digitalWrite(RELAY_PIN[i], relayState[i] ? LOW : HIGH);
}

void publishRelayState(int i) {
  char topic[80];
  sprintf(topic, "%s/relay/%d/state", baseTopic, i + 1);
  mqtt.publish(topic, relayState[i] ? "ON" : "OFF", true);  // retained
}

void setRelay(int i, bool on, bool persist = true) {
  relayState[i] = on;
  applyRelay(i);
  if (persist) saveRelay(i);
  publishRelayState(i);
  Serial.printf("[relay] %d -> %s\n", i + 1, on ? "ON" : "OFF");
}

// ---------------------------------------------------------------------
//  Jadwal
// ---------------------------------------------------------------------
void publishScheduleState(int i) {
  char on[8], off[8];
  minutesToHHMM(sched[i].onMin, on);
  minutesToHHMM(sched[i].offMin, off);
  StaticJsonDocument<96> doc;
  doc["enabled"] = sched[i].enabled;
  doc["on"]  = on;
  doc["off"] = off;
  char payload[96];
  serializeJson(doc, payload);
  char topic[80];
  sprintf(topic, "%s/schedule/%d/state", baseTopic, i + 1);
  mqtt.publish(topic, payload, true);  // retained
}

void evaluateSchedules() {
  struct tm t;
  if (!getLocalTime(&t, 100)) return;        // jam belum siap
  int nowMin = t.tm_hour * 60 + t.tm_min;
  if (nowMin == lastEvalMinute) return;      // sudah dievaluasi menit ini
  lastEvalMinute = nowMin;

  for (int i = 0; i < 2; i++) {
    if (!sched[i].enabled) continue;
    if (sched[i].onMin  == nowMin) setRelay(i, true);
    if (sched[i].offMin == nowMin) setRelay(i, false);
  }
}

// ---------------------------------------------------------------------
//  MQTT
// ---------------------------------------------------------------------
void handleScheduleSet(int i, byte* payload, unsigned int len) {
  StaticJsonDocument<128> doc;
  if (deserializeJson(doc, payload, len)) {
    Serial.println("[mqtt] jadwal JSON tidak valid");
    return;
  }
  sched[i].enabled = doc["enabled"] | false;
  sched[i].onMin   = hhmmToMinutes(doc["on"]  | "");
  sched[i].offMin  = hhmmToMinutes(doc["off"] | "");
  saveSchedule(i);
  publishScheduleState(i);
  lastEvalMinute = -1;                        // paksa evaluasi ulang
  Serial.printf("[sched] %d -> en=%d on=%d off=%d\n",
                i + 1, sched[i].enabled, sched[i].onMin, sched[i].offMin);
}

void onMessage(char* topic, byte* payload, unsigned int len) {
  // topic: <NS>/alat-NN/{relay|schedule}/{1|2}/set
  const char* sub = topic + strlen(baseTopic);   // -> "/relay/1/set"
  if (strncmp(sub, "/relay/", 7) == 0) {
    int idx = sub[7] - '1';                       // '1'->0, '2'->1
    if (idx < 0 || idx > 1) return;
    bool on = (len >= 2 && (payload[0] == 'O' || payload[0] == 'o')
                        && (payload[1] == 'N' || payload[1] == 'n'));
    setRelay(idx, on);
  } else if (strncmp(sub, "/schedule/", 10) == 0) {
    int idx = sub[10] - '1';
    if (idx < 0 || idx > 1) return;
    handleScheduleSet(idx, payload, len);
  }
}

void publishAllState() {
  for (int i = 0; i < 2; i++) {
    publishRelayState(i);
    publishScheduleState(i);
  }
}

void mqttConnect() {
  char statusTopic[72];
  sprintf(statusTopic, "%s/status", baseTopic);

  // Broker publik = anonim: kirim NULL bila user/pass kosong
  const char* user = strlen(MQTT_USER) ? MQTT_USER : nullptr;
  const char* pass = strlen(MQTT_PASS) ? MQTT_PASS : nullptr;

  // clientId harus unik di broker publik agar tak saling tendang
  char clientId[40];
  sprintf(clientId, "%s-%06X", deviceTag, (uint32_t)(ESP.getEfuseMac() & 0xFFFFFF));

  while (!mqtt.connected()) {
    Serial.print("[mqtt] menghubungkan... ");
    // connect(id, user, pass, willTopic, willQoS, willRetain, willMsg)
    if (mqtt.connect(clientId, user, pass,
                     statusTopic, 1, true, "offline")) {
      Serial.println("tersambung");
      mqtt.publish(statusTopic, "online", true);   // retained
      // subscribe perintah
      char sub[72];
      sprintf(sub, "%s/relay/+/set", baseTopic);    mqtt.subscribe(sub, 1);
      sprintf(sub, "%s/schedule/+/set", baseTopic); mqtt.subscribe(sub, 1);
      publishAllState();
    } else {
      Serial.printf("gagal rc=%d, coba lagi 3s\n", mqtt.state());
      delay(3000);
    }
  }
}

// ---------------------------------------------------------------------
//  Setup & loop
// ---------------------------------------------------------------------
void setup() {
  Serial.begin(115200);
  delay(200);

  // Identitas
  sprintf(deviceTag, "alat-%02d", DEVICE_ID);
  sprintf(baseTopic, "%s/%s", TOPIC_NS, deviceTag);
  Serial.printf("\n=== Smarthome %s ===\n", deviceTag);

  // Pulihkan state tersimpan SEBELUM menyalakan output (boot aman)
  loadState();
  for (int i = 0; i < 2; i++) {
    digitalWrite(RELAY_PIN[i], relayState[i] ? LOW : HIGH); // set latch dulu
    pinMode(RELAY_PIN[i], OUTPUT);                          // baru OUTPUT
    applyRelay(i);
  }

  // WiFi via captive portal
  char portal[28];
  sprintf(portal, "%s%02d", WIFI_PORTAL_PREFIX, DEVICE_ID);
  WiFiManager wm;
  wm.setConfigPortalTimeout(180);
  if (!wm.autoConnect(portal)) {
    Serial.println("[wifi] gagal, restart...");
    delay(2000);
    ESP.restart();
  }
  Serial.printf("[wifi] tersambung: %s\n", WiFi.localIP().toString().c_str());

  // NTP (jadwal butuh jam yang benar)
  configTime(TZ_OFFSET_SEC, DST_OFFSET_SEC, NTP_SERVER);
  struct tm t;
  Serial.print("[ntp] sinkron waktu");
  for (int i = 0; i < 20 && !getLocalTime(&t, 500); i++) Serial.print(".");
  Serial.println(getLocalTime(&t, 500) ? " ok" : " gagal (lanjut tanpa jam)");

  // MQTT (TLS tanpa validasi sertifikat — cukup untuk workshop)
  netClient.setInsecure();
  mqtt.setServer(MQTT_HOST, MQTT_PORT);
  mqtt.setCallback(onMessage);
  mqtt.setBufferSize(512);
  mqttConnect();
}

void loop() {
  if (!mqtt.connected()) mqttConnect();
  mqtt.loop();
  evaluateSchedules();
  delay(50);
}
