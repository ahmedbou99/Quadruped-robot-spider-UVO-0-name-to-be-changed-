/*
 * ESP32 MQTT Bridge — WOKWI TEST VERSION (plain MQTT, no TLS)
 *
 * Receives MQTT commands and prints them + the mapped Arduino command to Serial.
 * Connect to a public test broker or your local mosquitto.
 *
 * To test with your local broker:
 *   1. Find your host machine's LAN IP (e.g. 192.168.1.42)
 *   2. Set BROKER_HOST to that IP
 *   3. Set BROKER_PORT to 1883
 *   4. Make sure mosquitto.conf allows connections from the network
 */

#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

// ── WIFI ────────────────────────────
const char* WIFI_SSID = "Wokwi-GUEST";
const char* WIFI_PASS = "";           // Wokwi-GUEST has no password

// ── MQTT BROKER ─────────────────────
// EMQX public broker — works from any online simulator
const char*  BROKER_HOST = "broker.emqx.io";
const int    BROKER_PORT = 1883;
const char*  MQTT_USER   = "";        // test.mosquitto.org allows anonymous
const char*  MQTT_PASS   = "";
const char*  MQTT_CLIENT = "wokwi_esp32";

// ── TOPICS ──────────────────────────
const char* TOPIC_DIRECTION = "robot/spider/cmd/direction";
const char* TOPIC_SPEED     = "robot/spider/cmd/speed";
const char* TOPIC_GAIT      = "robot/spider/cmd/gait";
const char* TOPIC_ROTATION  = "robot/spider/cmd/rotation";

// ── ARDUINO COMMAND CODES ───────────
enum Cmd {
  REST         = 0,
  FORWARD      = 1,
  BACKWARD     = 2,
  ROTATE_RIGHT = 3,
  ROTATE_LEFT  = 4,
  SQUAT        = 5,
  WIGGLE       = 6,
};

// ── GLOBALS ─────────────────────────
WiFiClient   wifi;
PubSubClient mqtt(wifi);
int          currentSpeed = 50;

// ── FORWARD DECLS ───────────────────
void connectWiFi();
void connectMQTT();
void callback(char* topic, byte* payload, unsigned int len);
void handleDirection(const char* json);
void handleSpeed(const char* json);
void handleGait(const char* json);
void handleRotation(const char* json);

// ── SETUP ───────────────────────────
void setup() {
  Serial.begin(115200);
  Serial.println("\n=== ESP32 MQTT Bridge (Wokwi Test) ===");

  connectWiFi();

  mqtt.setServer(BROKER_HOST, BROKER_PORT);
  mqtt.setCallback(callback);
  mqtt.setBufferSize(2048);

  connectMQTT();

  Serial.println("Ready. Send MQTT messages to robot/spider/cmd/*");
  Serial.println("──────────────────────────────────────────");
}

// ── LOOP ────────────────────────────
void loop() {
  if (!mqtt.connected()) {
    delay(5000);
    connectMQTT();
  }
  mqtt.loop();
}

// ── WIFI ────────────────────────────
void connectWiFi() {
  Serial.printf("WiFi: connecting to %s", WIFI_SSID);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  int n = 0;
  while (WiFi.status() != WL_CONNECTED && n++ < 30) { delay(500); Serial.print("."); }
  if (WiFi.status() == WL_CONNECTED)
    Serial.printf("\nWiFi OK — IP: %s\n", WiFi.localIP().toString().c_str());
  else
    Serial.println("\nWiFi FAILED");
}

// ── MQTT ────────────────────────────
void connectMQTT() {
  Serial.printf("MQTT: connecting to %s:%d ... ", BROKER_HOST, BROKER_PORT);
  bool ok;
  if (strlen(MQTT_USER) > 0)
    ok = mqtt.connect(MQTT_CLIENT, MQTT_USER, MQTT_PASS);
  else
    ok = mqtt.connect(MQTT_CLIENT);

  if (ok) {
    Serial.println("OK");
    mqtt.subscribe(TOPIC_DIRECTION, 1);
    mqtt.subscribe(TOPIC_SPEED, 1);
    mqtt.subscribe(TOPIC_GAIT, 1);
    mqtt.subscribe(TOPIC_ROTATION, 1);
    Serial.println("Subscribed to robot/spider/cmd/*");
  } else {
    Serial.printf("FAILED (state=%d)\n", mqtt.state());
  }
}

// ── CALLBACK ────────────────────────
void callback(char* topic, byte* payload, unsigned int len) {
  char json[256] = {0};
  memcpy(json, payload, len < 255 ? len : 255);

  Serial.printf("\nMQTT ← [%s] %s\n", topic, json);

  if      (strcmp(topic, TOPIC_DIRECTION) == 0) handleDirection(json);
  else if (strcmp(topic, TOPIC_SPEED)     == 0) handleSpeed(json);
  else if (strcmp(topic, TOPIC_GAIT)      == 0) handleGait(json);
  else if (strcmp(topic, TOPIC_ROTATION)  == 0) handleRotation(json);
}

// ── DIRECTION: forward/backward/left/right/stop ──
void handleDirection(const char* json) {
  JsonDocument doc;
  if (deserializeJson(doc, json)) return;
  const char* dir = doc["direction"] | "";

  if      (strcmp(dir, "forward")  == 0) { Serial.println("  → Serial2: 1 (FORWARD)"); }
  else if (strcmp(dir, "backward") == 0) { Serial.println("  → Serial2: 2 (BACKWARD)"); }
  else if (strcmp(dir, "left")     == 0) { Serial.println("  → Serial2: 4 (ROTATE LEFT)"); }
  else if (strcmp(dir, "right")    == 0) { Serial.println("  → Serial2: 3 (ROTATE RIGHT)"); }
  else if (strcmp(dir, "squat")    == 0) { Serial.println("  → Serial2: 5 (SQUAT)"); }
  else if (strcmp(dir, "wiggle")   == 0) { Serial.println("  → Serial2: 6 (WIGGLE)"); }
  else if (strcmp(dir, "stop")     == 0) { Serial.println("  → Serial2: 0 (REST)"); }
  else Serial.printf("  ⚠ unknown direction '%s'\n", dir);
}

// ── SPEED: 0–100 ────────────────────
void handleSpeed(const char* json) {
  JsonDocument doc;
  if (deserializeJson(doc, json)) return;
  currentSpeed = constrain((int)(doc["speed"] | 50), 0, 100);
  Serial.printf("  → speed = %d%% (stored, not sent to Arduino)\n", currentSpeed);
}

// ── GAIT: tripod/wave/crawl/ripple ──
void handleGait(const char* json) {
  JsonDocument doc;
  if (deserializeJson(doc, json)) return;
  const char* gait = doc["gait"] | "";
  if      (strcmp(gait, "tripod") == 0) Serial.println("  → Serial2: 1 (FORWARD via tripod)");
  else if (strcmp(gait, "wave")   == 0) Serial.println("  → Serial2: 6 (WIGGLE via wave)");
  else if (strcmp(gait, "crawl")  == 0) Serial.println("  → Serial2: 1 (FORWARD via crawl)");
  else if (strcmp(gait, "ripple") == 0) Serial.println("  → Serial2: 5 (SQUAT via ripple)");
  else Serial.printf("  → gait = '%s'\n", gait);
}

// ── ROTATION: angle -180..180 ───────
void handleRotation(const char* json) {
  JsonDocument doc;
  if (deserializeJson(doc, json)) return;
  int angle = constrain((int)(doc["angle"] | 0), -180, 180);

  if      (angle > 0)  Serial.printf("  → [2] ROTATE RIGHT (3) [%d°]\n", angle);
  else if (angle < 0)  Serial.printf("  → [2] ROTATE LEFT (4) [%d°]\n", angle);
  else                 Serial.println("  → [2] REST (0)");
}
