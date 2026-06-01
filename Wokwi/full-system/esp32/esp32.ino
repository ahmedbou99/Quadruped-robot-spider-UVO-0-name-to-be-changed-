/*
 * ESP32 MQTT Bridge — FULL SYSTEM WOKWI TEST
 * Receives MQTT commands and forwards them to Arduino via Serial2 (UART).
 */

#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

const char* WIFI_SSID = "Wokwi-GUEST";
const char* WIFI_PASS = "";

// Use test.mosquitto.org for Wokwi cloud, or your LAN IP for local broker
const char*  BROKER_HOST = "test.mosquitto.org";
const int    BROKER_PORT = 1883;
const char*  MQTT_CLIENT = "wokwi_bridge";

const char* TP_DIRECTION = "robot/spider/cmd/direction";
const char* TP_SPEED     = "robot/spider/cmd/speed";
const char* TP_GAIT      = "robot/spider/cmd/gait";
const char* TP_ROTATION  = "robot/spider/cmd/rotation";

WiFiClient   wifi;
PubSubClient mqtt(wifi);

void connectWiFi() {
  Serial.printf("WiFi: %s", WIFI_SSID);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  int n = 0; while (WiFi.status() != WL_CONNECTED && n++ < 30) { delay(500); Serial.print("."); }
  Serial.println(WiFi.status() == WL_CONNECTED ? "\nWiFi OK" : "\nWiFi FAIL");
}

void connectMQTT() {
  Serial.printf("MQTT: %s:%d ... ", BROKER_HOST, BROKER_PORT);
  if (mqtt.connect(MQTT_CLIENT)) {
    Serial.println("OK");
    mqtt.subscribe(TP_DIRECTION, 1);
    mqtt.subscribe(TP_SPEED, 1);
    mqtt.subscribe(TP_GAIT, 1);
    mqtt.subscribe(TP_ROTATION, 1);
    Serial.println("Subscribed to robot/spider/cmd/*");
  } else Serial.printf("FAIL (rc=%d)\n", mqtt.state());
}

void callback(char* topic, byte* payload, unsigned int len) {
  char json[256] = {0};
  memcpy(json, payload, len < 255 ? len : 255);
  Serial.printf("MQTT ← [%s] %s\n", topic, json);

  JsonDocument doc;
  if (deserializeJson(doc, json)) return;
  const char* dir = doc["direction"] | "";

  if (strcmp(topic, TP_DIRECTION) == 0) {
    if      (strcmp(dir, "forward")  == 0) Serial2.println(1);
    else if (strcmp(dir, "backward") == 0) Serial2.println(2);
    else if (strcmp(dir, "left")     == 0) Serial2.println(4);
    else if (strcmp(dir, "right")    == 0) Serial2.println(3);
    else if (strcmp(dir, "squat")    == 0) Serial2.println(5);
    else if (strcmp(dir, "wiggle")   == 0) Serial2.println(6);
    else if (strcmp(dir, "stop")     == 0) Serial2.println(0);
    Serial.printf("  → Serial2 sent for '%s'\n", dir);
  }
  else if (strcmp(topic, TP_SPEED) == 0) {
    Serial.printf("  → speed=%d (stored)\n", (int)(doc["speed"] | 50));
  }
  else if (strcmp(topic, TP_GAIT) == 0) {
    Serial.printf("  → gait='%s' (logged)\n", doc["gait"] | "?");
  }
  else if (strcmp(topic, TP_ROTATION) == 0) {
    int angle = constrain((int)(doc["angle"] | 0), -180, 180);
    if (angle > 0)      Serial2.println(3);
    else if (angle < 0) Serial2.println(4);
    else                Serial2.println(0);
    Serial.printf("  → Serial2 sent for angle=%d\n", angle);
  }
}

void setup() {
  Serial.begin(115200);
  Serial2.begin(9600);  // UART to Arduino (TX=GPIO17, RX=GPIO16)
  Serial.println("=== ESP32 Bridge (Full System Wokwi) ===");
  connectWiFi();
  mqtt.setServer(BROKER_HOST, BROKER_PORT);
  mqtt.setCallback(callback);
  mqtt.setBufferSize(2048);
  connectMQTT();
}

void loop() {
  if (!mqtt.connected()) { delay(5000); connectMQTT(); }
  mqtt.loop();

  // Echo anything Arduino sends back
  while (Serial2.available()) Serial.write(Serial2.read());
}
