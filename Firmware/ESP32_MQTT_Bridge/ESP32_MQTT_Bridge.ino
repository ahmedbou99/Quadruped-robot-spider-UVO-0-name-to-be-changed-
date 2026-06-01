/*
 * ESP32 MQTT-to-Serial Bridge for Quadruped Spider Robot
 *
 * Connects to MQTT broker over TLS, subscribes to robot/spider/cmd/#,
 * translates JSON commands to integer codes sent over Serial2 to Arduino Uno.
 * Optionally captures camera frames (ESP32-CAM) and publishes them.
 *
 * Wiring:
 *   ESP32 TX2 (GPIO17) → Arduino RX (pin 0)   [use voltage divider: 3.3V → 5V safe]
 *   ESP32 RX2 (GPIO16) → Arduino TX (pin 1)   [ESP32 3.3V tolerant]
 *   ESP32 GND           → Arduino GND
 */

#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

// ───────────────────────────────────
//  ENABLE CAMERA (comment out if not using ESP32-CAM)
// ───────────────────────────────────
// #define ENABLE_CAMERA
#ifdef ENABLE_CAMERA
  #include "esp_camera.h"
#endif

// ───────────────────────────────────
//  WIFI CONFIGURATION
// ───────────────────────────────────
const char* WIFI_SSID = "YourWiFiSSID";
const char* WIFI_PASS = "YourWiFiPassword";

// ───────────────────────────────────
//  MQTT CONFIGURATION
// ───────────────────────────────────
const char* MQTT_HOST   = "mqtt.local";
const int   MQTT_PORT   = 8883;          // TLS
const char* MQTT_USER   = "spider_robot";
const char* MQTT_PASS   = "spider123";
const char* MQTT_CLIENT = "esp32_spider";

// ───────────────────────────────────
//  MQTT TOPICS
// ───────────────────────────────────
const char* TOPIC_CMD_DIRECTION = "robot/spider/cmd/direction";
const char* TOPIC_CMD_SPEED     = "robot/spider/cmd/speed";
const char* TOPIC_CMD_GAIT      = "robot/spider/cmd/gait";
const char* TOPIC_CMD_ROTATION  = "robot/spider/cmd/rotation";
const char* TOPIC_CAM_STREAM    = "robot/spider/cam/stream";
const char* TOPIC_STATUS        = "robot/spider/status";

// ───────────────────────────────────
//  CA CERTIFICATE (mosquitto/certs/ca.crt)
// ───────────────────────────────────
const char* CA_CERT = R"EOF(
-----BEGIN CERTIFICATE-----
MIIDCzCCAfOgAwIBAgIUX6JiyeLotvuJKz58PV+kH7YBqdswDQYJKoZIhvcNAQEL
BQAwFTETMBEGA1UEAwwKbXF0dC5sb2NhbDAeFw0yNjA2MDExMDEwNTJaFw0zNjA1
MjkxMDEwNTJaMBUxEzARBgNVBAMMCm1xdHQubG9jYWwwggEiMA0GCSqGSIb3DQEB
AQUAA4IBDwAwggEKAoIBAQChx6KOPA1W7cSBpIJ4UEPpyKgKIGTse0fe2Yqt7QYL
0M6dVlC3+obUDBi2/aezdMBpLGkFQ+Oe+MBX/xA3KPiToLD1GeNZKxszlSr4OXiw
X7nx3TQ3CTbcfAjQxTMWcB/Apd8EBKgc5ToWTeZ3DxrN7lgKnmXS8d1J+2lx5e6u
QifDRtyoBvffxdDvqxUkBxdoZWRryCSMQx7hae4nTk1SXvpq6Mq+XEg2oVmHhS8T
/Y2gtPlQSpmIMCRFe0mVgs4+R8JQBd651DTOHK67HVGgBbJtz58GCQCDagfz4RoZ
4O06MSzK5q76MAerliDEP/9TRW+iRfC+Us4U6AVlzTarAgMBAAGjUzBRMB0GA1Ud
DgQWBBRc05XLlgS4VUaFzshwvX0zprBxLTAfBgNVHSMEGDAWgBRc05XLlgS4VUaF
zshwvX0zprBxLTAPBgNVHRMBAf8EBTADAQH/MA0GCSqGSIb3DQEBCwUAA4IBAQA6
7DDnuYcReIBH75AZ6kdTtjfL1E8BwcbaWs74meHZ8l8j19xqUtulTFUKPabqFLzj
pKY92pMgUDZBDhVoCvdPKTswpLqgy948BWeiYrbVEyjjJSbvYf4Xpin3JfPYQkbv
d73VP3j8MDo4l6CxvgRCMSBHPGTiUpCqNdt7wPx7m9UhfrC4L22VfvGczbYcBX7/
IpliiNG8H+6JtkewmufIJFJFpidXjLQREF10S7TJjNLNXpzsFny7MXtwkZ5QPWAj
rHNYbCPyzaQanZH5owYMwumnitRGThehB0M4o9YIcKAg0nibC6a/5BRO0idN8ytr
ty8ThpxtY+1CJajxdkPN
-----END CERTIFICATE-----
)EOF";

// ───────────────────────────────────
//  ARDUINO SERIAL COMMAND CODES
// ───────────────────────────────────
enum ArduinoCommand {
  CMD_REST         = 0,
  CMD_FORWARD      = 1,
  CMD_BACKWARD     = 2,
  CMD_ROTATE_RIGHT = 3,
  CMD_ROTATE_LEFT  = 4,
  CMD_SQUAT        = 5,
  CMD_WIGGLE       = 6,
};

// ───────────────────────────────────
//  GLOBAL STATE
// ───────────────────────────────────
WiFiClientSecure wifiClient;
PubSubClient     mqtt(wifiClient);

unsigned long lastReconnectAttempt = 0;
unsigned long lastCameraFrame      = 0;
int           currentSpeed         = 50;   // 0–100

#ifdef ENABLE_CAMERA
  camera_fb_t* fb = nullptr;
#endif

// ───────────────────────────────────
//  FORWARD DECLARATIONS
// ───────────────────────────────────
void connectWiFi();
void connectMQTT();
void mqttCallback(char* topic, byte* payload, unsigned int length);
void sendArduinoCommand(int cmd);
void handleDirection(const char* json, unsigned int len);
void handleSpeed(const char* json, unsigned int len);
void handleGait(const char* json, unsigned int len);
void handleRotation(const char* json, unsigned int len);
#ifdef ENABLE_CAMERA
  void initCamera();
  void captureAndPublishFrame();
#endif

// ───────────────────────────────────
//  SETUP
// ───────────────────────────────────
void setup() {
  Serial.begin(115200);    // USB debug
  Serial2.begin(9600);     // UART to Arduino (GPIO17=TX, GPIO16=RX)

  Serial.println();
  Serial.println("=== ESP32 MQTT Bridge for Quadruped Spider ===");

  connectWiFi();

  // Configure TLS
  wifiClient.setCACert(CA_CERT);
  // wifiClient.setInsecure();  // uncomment to skip cert verification (testing only)

  mqtt.setServer(MQTT_HOST, MQTT_PORT);
  mqtt.setCallback(mqttCallback);
  mqtt.setBufferSize(4096);  // larger buffer for JSON payloads

  // LWT — broker publishes "offline" if we disconnect
  mqtt.setKeepAlive(15);

  connectMQTT();

  #ifdef ENABLE_CAMERA
    initCamera();
  #endif

  // Initial rest position
  sendArduinoCommand(CMD_REST);

  Serial.println("Bridge ready. Waiting for MQTT commands...");
}

// ───────────────────────────────────
//  LOOP
// ───────────────────────────────────
void loop() {
  if (!mqtt.connected()) {
    unsigned long now = millis();
    if (now - lastReconnectAttempt > 5000) {
      lastReconnectAttempt = now;
      Serial.println("MQTT disconnected — reconnecting...");
      connectMQTT();
    }
  }
  mqtt.loop();

  #ifdef ENABLE_CAMERA
    if (millis() - lastCameraFrame > 200) {  // 5 fps
      lastCameraFrame = millis();
      captureAndPublishFrame();
    }
  #endif
}

// ───────────────────────────────────
//  WIFI CONNECTION
// ───────────────────────────────────
void connectWiFi() {
  Serial.printf("Connecting to WiFi: %s\n", WIFI_SSID);
  WiFi.begin(WIFI_SSID, WIFI_PASS);

  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 30) {
    delay(500);
    Serial.print(".");
    attempts++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.printf("\nWiFi connected. IP: %s\n", WiFi.localIP().toString().c_str());
  } else {
    Serial.println("\nWiFi connection failed! Check credentials.");
  }
}

// ───────────────────────────────────
//  MQTT CONNECTION
// ───────────────────────────────────
void connectMQTT() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("No WiFi — cannot connect MQTT.");
    return;
  }

  Serial.printf("Connecting MQTT to %s:%d as '%s'...\n", MQTT_HOST, MQTT_PORT, MQTT_USER);

  // Connect with LWT: broker publishes "offline" on robot/spider/status if we drop
  bool connected = mqtt.connect(
    MQTT_CLIENT,
    MQTT_USER,
    MQTT_PASS,
    TOPIC_STATUS,                // LWT topic
    1,                           // LWT QoS
    true,                        // LWT retain
    "{\"status\":\"offline\"}"   // LWT payload
  );

  if (connected) {
    Serial.println("MQTT connected.");

    // Publish online status (retained so dashboard sees it on connect)
    mqtt.publish(TOPIC_STATUS, "{\"status\":\"online\"}", true);

    // Subscribe to all command topics
    mqtt.subscribe(TOPIC_CMD_DIRECTION, 1);
    mqtt.subscribe(TOPIC_CMD_SPEED, 1);
    mqtt.subscribe(TOPIC_CMD_GAIT, 1);
    mqtt.subscribe(TOPIC_CMD_ROTATION, 1);

    Serial.println("Subscribed to robot/spider/cmd/#");
  } else {
    Serial.printf("MQTT connect failed. State: %d\n", mqtt.state());
  }
}

// ───────────────────────────────────
//  MQTT CALLBACK — handles all incoming messages
// ───────────────────────────────────
void mqttCallback(char* topic, byte* payload, unsigned int length) {
  // Null-terminate the payload for JSON parsing
  char json[512] = {0};
  unsigned int copyLen = length < sizeof(json) - 1 ? length : sizeof(json) - 1;
  memcpy(json, payload, copyLen);

  Serial.printf("MQTT ← [%s] %s\n", topic, json);

  if (strcmp(topic, TOPIC_CMD_DIRECTION) == 0) {
    handleDirection(json, length);
  } else if (strcmp(topic, TOPIC_CMD_SPEED) == 0) {
    handleSpeed(json, length);
  } else if (strcmp(topic, TOPIC_CMD_GAIT) == 0) {
    handleGait(json, length);
  } else if (strcmp(topic, TOPIC_CMD_ROTATION) == 0) {
    handleRotation(json, length);
  } else {
    Serial.printf("Unknown topic: %s\n", topic);
  }
}

// ───────────────────────────────────
//  HANDLE DIRECTION COMMAND
//  Payload: { "direction": "forward"|"backward"|"left"|"right"|"stop" }
// ───────────────────────────────────
void handleDirection(const char* json, unsigned int len) {
  JsonDocument doc;
  DeserializationError err = deserializeJson(doc, json, len);

  if (err) {
    Serial.printf("JSON parse error: %s\n", err.c_str());
    return;
  }

  const char* dir = doc["direction"] | "";

  if (strcmp(dir, "forward") == 0) {
    Serial.println("→ Arduino: FORWARD (1)");
    sendArduinoCommand(CMD_FORWARD);
  } else if (strcmp(dir, "backward") == 0) {
    Serial.println("→ Arduino: BACKWARD (2)");
    sendArduinoCommand(CMD_BACKWARD);
  } else if (strcmp(dir, "left") == 0) {
    Serial.println("→ Arduino: ROTATE LEFT (4) [from direction:left]");
    sendArduinoCommand(CMD_ROTATE_LEFT);
  } else if (strcmp(dir, "right") == 0) {
    Serial.println("→ Arduino: ROTATE RIGHT (3) [from direction:right]");
    sendArduinoCommand(CMD_ROTATE_RIGHT);
  } else if (strcmp(dir, "squat") == 0) {
    Serial.println("→ Arduino: SQUAT (5)");
    sendArduinoCommand(CMD_SQUAT);
  } else if (strcmp(dir, "wiggle") == 0) {
    Serial.println("→ Arduino: WIGGLE (6)");
    sendArduinoCommand(CMD_WIGGLE);
  } else if (strcmp(dir, "stop") == 0) {
    Serial.println("→ Arduino: REST (0)");
    sendArduinoCommand(CMD_REST);
  } else {
    Serial.printf("Unknown direction: '%s'\n", dir);
  }
}

// ───────────────────────────────────
//  HANDLE SPEED COMMAND
//  Payload: { "speed": 0–100 }
// ───────────────────────────────────
void handleSpeed(const char* json, unsigned int len) {
  JsonDocument doc;
  DeserializationError err = deserializeJson(doc, json, len);

  if (err) {
    Serial.printf("JSON parse error: %s\n", err.c_str());
    return;
  }

  int speed = doc["speed"] | 50;
  speed = constrain(speed, 0, 100);
  currentSpeed = speed;

  Serial.printf("Speed set to %d%%\n", speed);
  // Speed affects timing on the Arduino side — could be sent as a separate
  // command if the Arduino firmware is extended to support variable speed.
}

// ───────────────────────────────────
//  HANDLE GAIT COMMAND
//  Payload: { "gait": "tripod"|"wave"|"crawl"|"ripple" }
// ───────────────────────────────────
void handleGait(const char* json, unsigned int len) {
  JsonDocument doc;
  DeserializationError err = deserializeJson(doc, json, len);

  if (err) {
    Serial.printf("JSON parse error: %s\n", err.c_str());
    return;
  }

  const char* gait = doc["gait"] | "";
  // Map gait names to Arduino commands
  if (strcmp(gait, "tripod") == 0)      sendArduinoCommand(CMD_FORWARD);
  else if (strcmp(gait, "wave") == 0)   sendArduinoCommand(CMD_WIGGLE);
  else if (strcmp(gait, "crawl") == 0)  sendArduinoCommand(CMD_FORWARD);
  else if (strcmp(gait, "ripple") == 0) sendArduinoCommand(CMD_SQUAT);
  Serial.printf("Gait '%s' → forwarded to Arduino\n", gait);
}

// ───────────────────────────────────
//  HANDLE ROTATION COMMAND
//  Payload: { "angle": -180 to 180 }  (clockwise positive)
// ───────────────────────────────────
void handleRotation(const char* json, unsigned int len) {
  JsonDocument doc;
  DeserializationError err = deserializeJson(doc, json, len);

  if (err) {
    Serial.printf("JSON parse error: %s\n", err.c_str());
    return;
  }

  int angle = doc["angle"] | 0;
  angle = constrain(angle, -180, 180);

  if (angle > 0) {
    Serial.printf("→ Arduino: ROTATE RIGHT (3) [angle=%d]\n", angle);
    sendArduinoCommand(CMD_ROTATE_RIGHT);
  } else if (angle < 0) {
    Serial.printf("→ Arduino: ROTATE LEFT (4) [angle=%d]\n", angle);
    sendArduinoCommand(CMD_ROTATE_LEFT);
  } else {
    Serial.println("→ Arduino: REST (0) [angle=0]");
    sendArduinoCommand(CMD_REST);
  }
}

// ───────────────────────────────────
//  SEND INTEGER COMMAND TO ARDUINO OVER SERIAL2
// ───────────────────────────────────
void sendArduinoCommand(int cmd) {
  Serial2.print('S');
  Serial2.println(currentSpeed);   // send speed first
  delay(5);
  Serial2.println(cmd);            // then the command
  Serial.printf("  Serial2 → S%d, %d\n", currentSpeed, cmd);
}

// ───────────────────────────────────
//  CAMERA (ESP32-CAM only)
// ───────────────────────────────────
#ifdef ENABLE_CAMERA

void initCamera() {
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer   = LEDC_TIMER_0;
  config.pin_d0       = 5;
  config.pin_d1       = 18;
  config.pin_d2       = 19;
  config.pin_d3       = 21;
  config.pin_d4       = 36;
  config.pin_d5       = 39;
  config.pin_d6       = 34;
  config.pin_d7       = 35;
  config.pin_xclk     = 0;
  config.pin_pclk     = 22;
  config.pin_vsync    = 25;
  config.pin_href     = 23;
  config.pin_sccb_sda = 26;
  config.pin_sccb_scl = 27;
  config.pin_pwdn     = 32;
  config.pin_reset    = -1;
  config.xclk_freq_hz = 20000000;
  config.pixel_format  = PIXFORMAT_JPEG;
  config.frame_size    = FRAMESIZE_QVGA;   // 320x240
  config.jpeg_quality  = 12;              // 0–63, lower = better
  config.fb_count      = 1;

  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed: 0x%x\n", err);
    return;
  }
  Serial.println("Camera initialized.");
}

void captureAndPublishFrame() {
  if (!mqtt.connected()) return;

  fb = esp_camera_fb_get();
  if (!fb) {
    Serial.println("Camera capture failed");
    return;
  }

  // Build JSON payload with base64 frame + timestamp
  JsonDocument doc;
  doc["frame"]     = "";  // placeholder — real base64 encoding would go here
  doc["timestamp"] = (unsigned long)(millis() / 1000);

  // For production: base64-encode fb->buf (fb->len bytes) into doc["frame"]
  // Using a library like libb64 or a manual base64 routine.
  // Skipping full base64 here to keep this file self-contained;
  // uncomment and integrate a base64 encoder for real camera streaming.

  char payload[512];
  serializeJson(doc, payload);

  mqtt.publish(TOPIC_CAM_STREAM, payload, 1);
  esp_camera_fb_return(fb);
}

#endif  // ENABLE_CAMERA
