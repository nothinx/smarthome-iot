// =====================================================================
//  Smarthome IoT — Firmware ESP32 DevKit v1
//  2 relay active-low + MQTT (broker lokal, plain) + NVS
//
//  Firmware ini IDENTIK untuk 10 alat. Yang diganti tiap unit hanya
//  DEVICE_ID di config.h.
//
//  Library (Library Manager):
//    - PubSubClient       (Nick O'Leary)
//  WiFi, WiFiClient, Preferences = bawaan core ESP32.
// =====================================================================
#include <WiFi.h>
#include <WiFiClient.h>
#include <PubSubClient.h>
#include <Preferences.h>
#include "config.h"

// ---------------------------------------------------------------------
//  Global
// ---------------------------------------------------------------------
WiFiClient       netClient;     // broker lokal = plain TCP (tanpa TLS)
PubSubClient     mqtt(netClient);
Preferences      prefs;

char deviceTag[12];        // "alat-01"
char baseTopic[64];        // "<NS>/alat-01"

const uint8_t RELAY_PIN[2] = { RELAY1_PIN, RELAY2_PIN };
bool relayState[2] = { false, false };

// ---------------------------------------------------------------------
//  Persistensi (NVS)
// ---------------------------------------------------------------------
void loadState() {
  prefs.begin("smarthome", true);
  relayState[0] = prefs.getBool("r0", false);
  relayState[1] = prefs.getBool("r1", false);
  prefs.end();
}

void saveRelay(int i) {
  prefs.begin("smarthome", false);
  prefs.putBool(i == 0 ? "r0" : "r1", relayState[i]);
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
//  MQTT
// ---------------------------------------------------------------------
void onMessage(char* topic, byte* payload, unsigned int len) {
  // topic: <NS>/alat-NN/relay/{1|2}/set
  const char* sub = topic + strlen(baseTopic);   // -> "/relay/1/set"
  if (strncmp(sub, "/relay/", 7) == 0) {
    int idx = sub[7] - '1';                       // '1'->0, '2'->1
    if (idx < 0 || idx > 1) return;
    bool on = (len >= 2 && (payload[0] == 'O' || payload[0] == 'o')
                        && (payload[1] == 'N' || payload[1] == 'n'));
    setRelay(idx, on);
  }
}

void publishAllState() {
  for (int i = 0; i < 2; i++) publishRelayState(i);
}

void mqttConnect() {
  char statusTopic[72];
  sprintf(statusTopic, "%s/status", baseTopic);

  // Broker lokal anonim: kirim NULL bila user/pass kosong
  const char* user = strlen(MQTT_USER) ? MQTT_USER : nullptr;
  const char* pass = strlen(MQTT_PASS) ? MQTT_PASS : nullptr;

  // clientId unik agar tak saling tendang di broker
  char clientId[40];
  sprintf(clientId, "%s-%06X", deviceTag, (uint32_t)(ESP.getEfuseMac() & 0xFFFFFF));

  while (!mqtt.connected()) {
    Serial.print("[mqtt] menghubungkan... ");
    // connect(id, user, pass, willTopic, willQoS, willRetain, willMsg)
    if (mqtt.connect(clientId, user, pass,
                     statusTopic, 1, true, "offline")) {
      Serial.println("tersambung");
      mqtt.publish(statusTopic, "online", true);   // retained
      char sub[72];
      sprintf(sub, "%s/relay/+/set", baseTopic);
      mqtt.subscribe(sub, 1);
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

  // WiFi (AP lokal, kredensial tetap di config.h)
  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(true);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  Serial.printf("[wifi] menyambung ke %s", WIFI_SSID);
  for (int i = 0; i < 60 && WiFi.status() != WL_CONNECTED; i++) {
    delay(500);
    Serial.print(".");
  }
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println(" gagal, restart...");
    delay(2000);
    ESP.restart();
  }
  Serial.printf("\n[wifi] tersambung: %s\n", WiFi.localIP().toString().c_str());

  // MQTT ke broker lokal (plain TCP, port 1883)
  mqtt.setServer(MQTT_HOST, MQTT_PORT);
  mqtt.setCallback(onMessage);
  mqtt.setBufferSize(512);
  mqttConnect();
}

void loop() {
  if (!mqtt.connected()) mqttConnect();
  mqtt.loop();
  delay(50);
}
