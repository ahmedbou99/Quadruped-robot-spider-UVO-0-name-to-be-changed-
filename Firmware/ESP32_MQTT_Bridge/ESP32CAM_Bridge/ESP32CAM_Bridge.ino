/*
 * ESP32-CAM — MQTT Bridge + Camera Stream for Quadruped Spider Robot
 *
 * Board: AI-Thinker ESP32-CAM
 * Camera: OV2640
 *
 * What it does:
 *   1. Connects to WiFi + MQTT broker (your local mosquitto)
 *   2. Subscribes to robot/spider/cmd/# and forwards commands to Arduino via Serial2
 *   3. Captures camera frames, base64-encodes, publishes to robot/spider/cam/stream
 *   4. LWT on robot/spider/status for online/offline detection
 *
 * Wiring (ESP32-CAM → Arduino Uno):
 *   GPIO14 (TX2)  → Arduino pin 0 (RX)   [use 2.2kΩ/3.3kΩ voltage divider]
 *   GPIO15 (RX2)  → Arduino pin 1 (TX)
 *   GND           → Arduino GND
 *   +5V           → external 5V ( don't power cam from Arduino 5V pin)
 */

#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include "esp_camera.h"

// ───────────────────────────────────
//  WIFI — CHANGE THESE
// ───────────────────────────────────
const char* WIFI_SSID = "YourWiFiSSID";
const char* WIFI_PASS = "YourWiFiPassword";

// ───────────────────────────────────
//  MQTT BROKER — local mosquitto
// ───────────────────────────────────
const char*  MQTT_HOST   = "mqtt.local";
const int    MQTT_PORT   = 1883;           // plain MQTT (use 8883 for TLS)
const char*  MQTT_USER   = "spider_robot";
const char*  MQTT_PASS   = "spider123";
const char*  MQTT_CLIENT = "esp32cam_spider";

// ───────────────────────────────────
//  MQTT TOPICS
// ───────────────────────────────────
const char* TP_DIRECTION = "robot/spider/cmd/direction";
const char* TP_SPEED     = "robot/spider/cmd/speed";
const char* TP_GAIT      = "robot/spider/cmd/gait";
const char* TP_ROTATION  = "robot/spider/cmd/rotation";
const char* TP_CAM       = "robot/spider/cam/stream";
const char* TP_STATUS    = "robot/spider/status";

// ───────────────────────────────────
//  ARDUINO COMMAND CODES
// ───────────────────────────────────
enum {
  CMD_REST         = 0,
  CMD_FORWARD      = 1,
  CMD_BACKWARD     = 2,
  CMD_ROTATE_RIGHT = 3,
  CMD_ROTATE_LEFT  = 4,
  CMD_SQUAT        = 5,
  CMD_WIGGLE       = 6,
};

// ───────────────────────────────────
//  ESP32-CAM PIN DEFINITIONS (AI-Thinker)
// ───────────────────────────────────
#define CAM_PIN_PWDN    32
#define CAM_PIN_RESET   -1
#define CAM_PIN_XCLK     0
#define CAM_PIN_SIOD    26
#define CAM_PIN_SIOC    27
#define CAM_PIN_D7      35
#define CAM_PIN_D6      34
#define CAM_PIN_D5      39
#define CAM_PIN_D4      36
#define CAM_PIN_D3      21
#define CAM_PIN_D2      19
#define CAM_PIN_D1      18
#define CAM_PIN_D0       5
#define CAM_PIN_VSYNC   25
#define CAM_PIN_HREF    23
#define CAM_PIN_PCLK    22

// Serial2 pins to Arduino
// RX2=14 receives from Arduino TX, TX2=13 sends to Arduino RX
#define ARDUINO_RX  14   // ESP32-CAM GPIO14 ← Arduino pin 1 (TX)
#define ARDUINO_TX  13   // ESP32-CAM GPIO13 → Arduino pin 0 (RX)

// ───────────────────────────────────
//  CAMERA SETTINGS
// ───────────────────────────────────
#define CAM_FRAME_INTERVAL 333   // ms between frames (~3 fps)
#define CAM_JPEG_QUALITY   12    // 0-63, lower=better quality

// ───────────────────────────────────
//  TLS (uncomment for port 8883)
// ───────────────────────────────────
// #define USE_TLS
#ifdef USE_TLS
  #include "ca_cert.h"  // embed ca.crt as a char array
  WiFiClientSecure wifiClient;
#else
  WiFiClient wifiClient;
#endif

// ───────────────────────────────────
//  GLOBALS
// ───────────────────────────────────
PubSubClient  mqtt(wifiClient);
unsigned long lastReconnect = 0;
unsigned long lastFrame     = 0;
int           currentSpeed  = 50;
camera_fb_t*  fb            = nullptr;

// ───────────────────────────────────
//  BASE64 ENCODER (inline, no library needed)
// ───────────────────────────────────
static const char b64table[] =
  "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

String base64Encode(const uint8_t* data, size_t len) {
  String out;
  out.reserve((len + 2) / 3 * 4);
  for (size_t i = 0; i < len; i += 3) {
    uint32_t triple = (uint32_t)data[i] << 16;
    if (i + 1 < len) triple |= (uint32_t)data[i + 1] << 8;
    if (i + 2 < len) triple |= data[i + 2];
    out += b64table[(triple >> 18) & 0x3F];
    out += b64table[(triple >> 12) & 0x3F];
    out += (i + 1 < len) ? b64table[(triple >> 6) & 0x3F] : '=';
    out += (i + 2 < len) ? b64table[triple & 0x3F] : '=';
  }
  return out;
}

// ───────────────────────────────────
//  FORWARD DECLARATIONS
// ───────────────────────────────────
void connectWiFi();
void connectMQTT();
bool initCamera();
void captureAndPublish();
void sendCommand(int cmd);
void mqttCallback(char* topic, byte* payload, unsigned int len);
void handleDirection(JsonDocument& doc);
void handleSpeed(JsonDocument& doc);
void handleGait(JsonDocument& doc);
void handleRotation(JsonDocument& doc);

// ═════════════════════════════════════
//  SETUP
// ═════════════════════════════════════
void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println("\n=== ESP32-CAM Spider Bridge ===");

  // ── Camera init ──
  if (!initCamera()) {
    Serial.println("Camera failed — running bridge only.");
  }

  // ── WiFi ──
  connectWiFi();

  // ── MQTT ──
#ifdef USE_TLS
  wifiClient.setInsecure();  // skip cert verification (or use setCACert)
#endif
  mqtt.setServer(MQTT_HOST, MQTT_PORT);
  mqtt.setCallback(mqttCallback);
  mqtt.setBufferSize(36864);  // match camBuf for QVGA frames
  connectMQTT();

  // ── Serial2 to Arduino ──
  Serial2.begin(9600, SERIAL_8N1, ARDUINO_RX, ARDUINO_TX);
  // wait a beat then send rest
  delay(200);
  sendCommand(CMD_REST);

  Serial.println("Ready.\n");
}

// ═════════════════════════════════════
//  LOOP
// ═════════════════════════════════════
void loop() {
  // MQTT reconnect
  if (!mqtt.connected()) {
    if (millis() - lastReconnect > 5000) {
      lastReconnect = millis();
      connectMQTT();
    }
  }
  mqtt.loop();

  // Camera capture
  if (millis() - lastFrame > CAM_FRAME_INTERVAL) {
    lastFrame = millis();
    captureAndPublish();
  }
}

// ───────────────────────────────────
//  WIFI
// ───────────────────────────────────
void connectWiFi() {
  Serial.printf("WiFi: %s", WIFI_SSID);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  int n = 0;
  while (WiFi.status() != WL_CONNECTED && n++ < 30) { delay(500); Serial.print("."); }
  if (WiFi.status() == WL_CONNECTED)
    Serial.printf("\nWiFi OK — IP: %s\n", WiFi.localIP().toString().c_str());
  else
    Serial.println("\nWiFi FAILED");
}

// ───────────────────────────────────
//  MQTT CONNECT + SUBSCRIBE
// ───────────────────────────────────
void connectMQTT() {
  if (WiFi.status() != WL_CONNECTED) return;

  Serial.printf("MQTT: %s:%d ... ", MQTT_HOST, MQTT_PORT);
  bool ok = mqtt.connect(
    MQTT_CLIENT,
    MQTT_USER, MQTT_PASS,
    TP_STATUS, 1, true, "{\"status\":\"offline\"}"  // LWT
  );

  if (ok) {
    Serial.println("OK");
    mqtt.publish(TP_STATUS, "{\"status\":\"online\"}", true);
    mqtt.subscribe(TP_DIRECTION, 1);
    mqtt.subscribe(TP_SPEED, 1);
    mqtt.subscribe(TP_GAIT, 1);
    mqtt.subscribe(TP_ROTATION, 1);
    Serial.println("Subscribed to robot/spider/cmd/#");
  } else {
    Serial.printf("FAILED (state=%d)\n", mqtt.state());
  }
}

// ───────────────────────────────────
//  CAMERA INIT
// ───────────────────────────────────
bool initCamera() {
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer   = LEDC_TIMER_0;
  config.pin_d0       = CAM_PIN_D0;
  config.pin_d1       = CAM_PIN_D1;
  config.pin_d2       = CAM_PIN_D2;
  config.pin_d3       = CAM_PIN_D3;
  config.pin_d4       = CAM_PIN_D4;
  config.pin_d5       = CAM_PIN_D5;
  config.pin_d6       = CAM_PIN_D6;
  config.pin_d7       = CAM_PIN_D7;
  config.pin_xclk     = CAM_PIN_XCLK;
  config.pin_pclk     = CAM_PIN_PCLK;
  config.pin_vsync    = CAM_PIN_VSYNC;
  config.pin_href     = CAM_PIN_HREF;
  config.pin_sccb_sda = CAM_PIN_SIOD;
  config.pin_sccb_scl = CAM_PIN_SIOC;
  config.pin_pwdn     = CAM_PIN_PWDN;
  config.pin_reset    = CAM_PIN_RESET;
  config.xclk_freq_hz = 20000000;
  config.pixel_format  = PIXFORMAT_JPEG;
  config.frame_size    = FRAMESIZE_QVGA;   // 320×240
  config.jpeg_quality  = CAM_JPEG_QUALITY;
  config.fb_count      = 1;

  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed: 0x%x\n", err);
    return false;
  }

  // Camera settings
  sensor_t* s = esp_camera_sensor_get();
  if (s) {
    s->set_brightness(s, 0);     // -2 to 2
    s->set_contrast(s, 0);       // -2 to 2
    s->set_saturation(s, 0);     // -2 to 2
    s->set_whitebal(s, 1);       // auto white balance
    s->set_awb_gain(s, 1);       // auto gain
    s->set_exposure_ctrl(s, 1);  // auto exposure
    s->set_gain_ctrl(s, 1);      // auto gain control
    s->set_quality(s, CAM_JPEG_QUALITY);
  }

  Serial.println("Camera OK — OV2640 QVGA");
  return true;
}

// ───────────────────────────────────
//  CAMERA CAPTURE + PUBLISH
// ───────────────────────────────────
// Static buffer for camera JSON payload (12KB, in BSS not stack)
static char camBuf[36864];  // 36KB for QVGA base64 frames

void captureAndPublish() {
  if (!mqtt.connected()) return;

  camera_fb_t* fb = esp_camera_fb_get();
  if (!fb || fb->len == 0) {
    if (fb) esp_camera_fb_return(fb);
    return;
  }

  // Calculate sizes
  size_t b64Max = (fb->len + 2) / 3 * 4;  // worst-case base64 size
  size_t jsonHead = 40;                     // {"frame":"
  size_t jsonTail = 30;                     // ","timestamp":1234567890}
  size_t total = jsonHead + b64Max + jsonTail;

  if (total >= sizeof(camBuf) - 100) {
    // Frame too large, skip
    esp_camera_fb_return(fb);
    return;
  }

  // Write JSON header
  int pos = snprintf(camBuf, sizeof(camBuf), "{\"frame\":\"");

  // Base64-encode JPEG directly into camBuf
  for (size_t i = 0; i < fb->len; i += 3) {
    uint32_t triple = (uint32_t)fb->buf[i] << 16;
    if (i + 1 < fb->len) triple |= (uint32_t)fb->buf[i + 1] << 8;
    if (i + 2 < fb->len) triple |= fb->buf[i + 2];
    camBuf[pos++] = b64table[(triple >> 18) & 0x3F];
    camBuf[pos++] = b64table[(triple >> 12) & 0x3F];
    camBuf[pos++] = (i + 1 < fb->len) ? b64table[(triple >> 6) & 0x3F] : '=';
    camBuf[pos++] = (i + 2 < fb->len) ? b64table[triple & 0x3F] : '=';
  }

  // Write JSON tail
  pos += snprintf(camBuf + pos, sizeof(camBuf) - pos,
    "\",\"timestamp\":%lu}", (unsigned long)(millis() / 1000));

  esp_camera_fb_return(fb);

  // Publish
  if (mqtt.publish(TP_CAM, (uint8_t*)camBuf, pos, false)) {
    Serial.printf("Cam: %d bytes\n", pos);
  }
}

// ───────────────────────────────────
//  SEND COMMAND TO ARDUINO
//  Protocol: S<speed>\n<command>\n
// ───────────────────────────────────
void sendCommand(int cmd) {
  Serial2.print('S');
  Serial2.println(currentSpeed);
  delay(2);
  Serial2.println(cmd);
  Serial.printf("  → Arduino: S%d, %d\n", currentSpeed, cmd);
}

// ───────────────────────────────────
//  MQTT CALLBACK
// ───────────────────────────────────
void mqttCallback(char* topic, byte* payload, unsigned int length) {
  char json[512] = {0};
  memcpy(json, payload, length < 511 ? length : 511);

  Serial.printf("MQTT ← [%s] %s\n", topic, json);

  JsonDocument doc;
  if (deserializeJson(doc, json, length)) {
    Serial.println("  JSON parse error");
    return;
  }

  if      (strcmp(topic, TP_DIRECTION) == 0) handleDirection(doc);
  else if (strcmp(topic, TP_SPEED)     == 0) handleSpeed(doc);
  else if (strcmp(topic, TP_GAIT)      == 0) handleGait(doc);
  else if (strcmp(topic, TP_ROTATION)  == 0) handleRotation(doc);
}

// ───────────────────────────────────
//  DIRECTION → Arduino command
// ───────────────────────────────────
void handleDirection(JsonDocument& doc) {
  const char* dir = doc["direction"] | "";
  if      (strcmp(dir, "forward")  == 0) sendCommand(CMD_FORWARD);
  else if (strcmp(dir, "backward") == 0) sendCommand(CMD_BACKWARD);
  else if (strcmp(dir, "left")     == 0) sendCommand(CMD_ROTATE_LEFT);
  else if (strcmp(dir, "right")    == 0) sendCommand(CMD_ROTATE_RIGHT);
  else if (strcmp(dir, "squat")    == 0) sendCommand(CMD_SQUAT);
  else if (strcmp(dir, "wiggle")   == 0) sendCommand(CMD_WIGGLE);
  else if (strcmp(dir, "stop")     == 0) sendCommand(CMD_REST);
  else Serial.printf("  unknown direction: '%s'\n", dir);
}

// ───────────────────────────────────
//  SPEED
// ───────────────────────────────────
void handleSpeed(JsonDocument& doc) {
  currentSpeed = constrain((int)(doc["speed"] | 50), 0, 100);
  Serial.printf("  speed = %d%%\n", currentSpeed);
}

// ───────────────────────────────────
//  GAIT → Arduino command
// ───────────────────────────────────
void handleGait(JsonDocument& doc) {
  const char* gait = doc["gait"] | "";
  if      (strcmp(gait, "tripod") == 0) sendCommand(CMD_FORWARD);
  else if (strcmp(gait, "wave")   == 0) sendCommand(CMD_WIGGLE);
  else if (strcmp(gait, "crawl")  == 0) sendCommand(CMD_FORWARD);
  else if (strcmp(gait, "ripple") == 0) sendCommand(CMD_SQUAT);
  else Serial.printf("  gait = '%s'\n", gait);
}

// ───────────────────────────────────
//  ROTATION
// ───────────────────────────────────
void handleRotation(JsonDocument& doc) {
  int angle = constrain((int)(doc["angle"] | 0), -180, 180);
  if      (angle > 0) sendCommand(CMD_ROTATE_RIGHT);
  else if (angle < 0) sendCommand(CMD_ROTATE_LEFT);
  else                sendCommand(CMD_REST);
}
