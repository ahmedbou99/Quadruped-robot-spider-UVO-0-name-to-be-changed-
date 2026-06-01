/*
 * ESP32 MQTT Bridge — STANDALONE (no external libraries needed)
 * Paste this entire file into velxio.dev — uses only built-in WiFi.h
 *
 * Implements a minimal MQTT client + simple JSON parser inline.
 * Connects to broker.emqx.io, subscribes to robot/spider/cmd/#,
 * and prints received commands to Serial.
 */

#include <WiFi.h>

// ── CONFIG ──────────────────────────
const char* WIFI_SSID = "Wokwi-GUEST";
const char* WIFI_PASS = "";
const char* BROKER_HOST = "broker.emqx.io";
const int   BROKER_PORT = 1883;
const char* CLIENT_ID   = "velxio_esp32";

// MQTT topics to subscribe to
const char* SUBS[] = {
  "robot/spider/cmd/direction",
  "robot/spider/cmd/speed",
  "robot/spider/cmd/gait",
  "robot/spider/cmd/rotation"
};
const int SUB_COUNT = 4;

WiFiClient tcp;

// ── MINIMAL JSON HELPERS ────────────
// Extracts string value for a key: {"direction":"forward"} → "forward"
String jsonGetString(const String& json, const char* key) {
  String pat = String("\"") + key + "\":\"";
  int start = json.indexOf(pat);
  if (start < 0) return "";
  start += pat.length();
  int end = json.indexOf("\"", start);
  if (end < 0) return "";
  return json.substring(start, end);
}

// Extracts int value for a key: {"speed":75} → 75
int jsonGetInt(const String& json, const char* key) {
  String pat = String("\"") + key + "\":";
  int start = json.indexOf(pat);
  if (start < 0) return 0;
  start += pat.length();
  return json.substring(start).toInt();
}

// ── MINIMAL MQTT ────────────────────

// Write a big-endian 16-bit length
void writeLen(WiFiClient& c, uint16_t len) {
  c.write((len >> 8) & 0xFF);
  c.write(len & 0xFF);
}

// Build and send an MQTT CONNECT packet
bool mqttConnect() {
  // Fixed header: CONNECT (0x10), remaining length
  uint8_t protoName[] = {0x00, 0x04, 'M', 'Q', 'T', 'T'};
  uint8_t varHeader[] = {
    0x04,       // protocol level (MQTT 3.1.1)
    0x02,       // connect flags: clean session
    0x00, 0x3C  // keepalive = 60s
  };

  int remLen = sizeof(protoName) + sizeof(varHeader)
             + 2 + strlen(CLIENT_ID);  // client ID (length-prefixed)

  // Fixed header
  tcp.write(0x10);                           // CONNECT
  if (remLen < 128) {
    tcp.write((uint8_t)remLen);
  } else {
    tcp.write((uint8_t)((remLen % 128) | 0x80));
    tcp.write((uint8_t)(remLen / 128));
  }

  // Variable header: protocol name + flags + keepalive
  tcp.write(protoName, sizeof(protoName));
  tcp.write(varHeader, sizeof(varHeader));

  // Payload: client ID
  writeLen(tcp, strlen(CLIENT_ID));
  tcp.print(CLIENT_ID);

  tcp.flush();

  // Wait for CONNACK (should be 4 bytes: 0x20, 0x02, 0x00, 0x00)
  unsigned long t = millis();
  while (!tcp.available() && (millis() - t) < 5000) delay(10);
  if (!tcp.available()) return false;
  uint8_t ack[4];
  tcp.readBytes(ack, 4);
  return (ack[0] == 0x20 && ack[3] == 0x00);  // CONNACK accepted
}

// Send SUBSCRIBE for a topic
void mqttSubscribe(const char* topic) {
  uint16_t pktId = 1;

  int remLen = 2 + 2 + strlen(topic) + 1;  // pktId + topicLen + topic + QoS
  tcp.write(0x82);  // SUBSCRIBE header
  if (remLen < 128) tcp.write((uint8_t)remLen);
  else { tcp.write((uint8_t)(remLen | 0x80)); tcp.write(1); }
  writeLen(tcp, pktId);
  writeLen(tcp, strlen(topic));
  tcp.print(topic);
  tcp.write(0x01);  // QoS 1
  tcp.flush();
  // SUBACK will arrive — we just consume it
  delay(100);
  while (tcp.available()) tcp.read();
}

// Read an MQTT packet — returns the payload as String, sets *topic
String mqttRead(String* topic) {
  while (tcp.available()) {
    uint8_t hdr = tcp.read();
    uint8_t type = hdr & 0xF0;

    // Read remaining length
    uint32_t remLen = 0;
    uint8_t mul = 1;
    for (int i = 0; i < 4; i++) {
      uint8_t b = tcp.read();
      remLen += (b & 0x7F) * mul;
      mul *= 128;
      if (!(b & 0x80)) break;
    }

    uint8_t buf[remLen];
    tcp.readBytes(buf, remLen);

    if (type == 0x30) {  // PUBLISH
      uint16_t tlen = (buf[0] << 8) | buf[1];
      char t[tlen + 1];
      memcpy(t, buf + 2, tlen);
      t[tlen] = 0;
      if (topic) *topic = String(t);
      uint32_t plen = remLen - 2 - tlen;
      char p[plen + 1];
      memcpy(p, buf + 2 + tlen, plen);
      p[plen] = 0;
      return String(p);
    }
  }
  return "";
}

// ── MAIN SETUP ──────────────────────
void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println("\n=== ESP32 MQTT Bridge (standalone) ===");

  // WiFi
  Serial.printf("WiFi: connecting to %s", WIFI_SSID);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  int n = 0;
  while (WiFi.status() != WL_CONNECTED && n++ < 40) { delay(500); Serial.print("."); }
  if (WiFi.status() == WL_CONNECTED)
    Serial.printf("\nWiFi OK — IP: %s\n", WiFi.localIP().toString().c_str());
  else { Serial.println("\nWiFi FAILED"); return; }

  // MQTT connect
  Serial.printf("MQTT: connecting to %s:%d ... ", BROKER_HOST, BROKER_PORT);
  if (!tcp.connect(BROKER_HOST, BROKER_PORT)) {
    Serial.println("TCP FAILED");
    return;
  }
  if (!mqttConnect()) {
    Serial.println("MQTT AUTH FAILED");
    return;
  }
  Serial.println("OK");

  // Subscribe
  for (int i = 0; i < SUB_COUNT; i++) {
    Serial.printf("  Subscribing to %s\n", SUBS[i]);
    mqttSubscribe(SUBS[i]);
  }

  Serial.println("Ready. Dashboard → broker.emqx.io:8083 → here.");
  Serial.println("──────────────────────────────────────────");
}

// ── MAIN LOOP ───────────────────────
void loop() {
  if (!tcp.connected()) {
    Serial.println("MQTT disconnected — reconnecting...");
    delay(5000);
    if (tcp.connect(BROKER_HOST, BROKER_PORT)) {
      mqttConnect();
      for (int i = 0; i < SUB_COUNT; i++) mqttSubscribe(SUBS[i]);
      Serial.println("Reconnected.");
    }
  }

  String topic;
  String payload = mqttRead(&topic);
  if (payload.length() == 0) { delay(10); return; }

  Serial.printf("\nMQTT ← [%s] %s\n", topic.c_str(), payload.c_str());

  if (topic.endsWith("direction")) {
    String dir = jsonGetString(payload, "direction");
    if      (dir == "forward")  Serial.println("  → Serial2: 1 (FORWARD)");
    else if (dir == "backward") Serial.println("  → Serial2: 2 (BACKWARD)");
    else if (dir == "left")     Serial.println("  → Serial2: 4 (ROTATE LEFT)");
    else if (dir == "right")    Serial.println("  → Serial2: 3 (ROTATE RIGHT)");
    else if (dir == "squat")    Serial.println("  → Serial2: 5 (SQUAT)");
    else if (dir == "wiggle")   Serial.println("  → Serial2: 6 (WIGGLE)");
    else if (dir == "stop")     Serial.println("  → Serial2: 0 (REST)");
    else                        Serial.printf("  ⚠ unknown: %s\n", dir.c_str());
  }
  else if (topic.endsWith("speed")) {
    int s = jsonGetInt(payload, "speed");
    Serial.printf("  → speed = %d%%\n", s);
  }
  else if (topic.endsWith("gait")) {
    String g = jsonGetString(payload, "gait");
    if      (g == "tripod") Serial.println("  → Serial2: 1 (FORWARD via tripod)");
    else if (g == "wave")   Serial.println("  → Serial2: 6 (WIGGLE via wave)");
    else if (g == "crawl")  Serial.println("  → Serial2: 1 (FORWARD via crawl)");
    else if (g == "ripple") Serial.println("  → Serial2: 5 (SQUAT via ripple)");
    else                    Serial.printf("  → gait = '%s'\n", g.c_str());
  }
  else if (topic.endsWith("rotation")) {
    int a = jsonGetInt(payload, "angle");
    if      (a > 0)  Serial.printf("  → ROTATE RIGHT (3) [%d°]\n", a);
    else if (a < 0)  Serial.printf("  → ROTATE LEFT (4) [%d°]\n", a);
    else             Serial.println("  → REST (0)");
  }
}
