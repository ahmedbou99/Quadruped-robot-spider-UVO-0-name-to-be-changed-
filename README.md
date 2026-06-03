# Quadruped Spider Robot

Four-legged spider robot based on the Otto DIY quadruped. Arduino Uno drives 8 servos. ESP32-CAM provides MQTT bridge + camera streaming. Web dashboard for remote control.

## Quick Start

```bash
git clone https://github.com/ahmedbou99/Quadruped-robot-spider-UVO-0-name-to-be-changed-.git
cd Quadruped-robot-spider-UVO-0-name-to-be-changed-

# 1. Run setup (generates TLS certs, MQTT passwords, config)
chmod +x setup.sh
./setup.sh

# 2. Add mqtt.local to hosts (if setup.sh told you to)
sudo sh -c 'echo "127.0.0.1 mqtt.local" >> /etc/hosts'

# 3. Start MQTT broker
mosquitto -c mosquitto/mosquitto-local.conf -d

# 4. Start web dashboard
cd Web && npm install && npm run dev
# Opens at http://localhost:5173

# 5. Flash Arduino Uno
# Open: Firmware/ArduinoVer0/Spider_class_test/Spider_class_test.ino
# Board: Arduino Uno

# 6. Flash ESP32-CAM
# Open: Firmware/ESP32_MQTT_Bridge/ESP32CAM_Bridge.ino
# Board: AI-Thinker ESP32-CAM
# Edit WiFi credentials at top of file FIRST and Change local.mqtt to the user Ip
```

## Architecture

```
┌──────────────┐     MQTT/WebSocket      ┌──────────────┐
│   Browser    │◄────────────────────────→│   Mosquitto  │
│  (Dashboard) │    ws://mqtt.local:9002  │   (Broker)   │
└──────────────┘                         └──────┬───────┘
                                                │ MQTT
                                         ┌──────┴───────┐
                                         │   ESP32-CAM  │
                                         │  (MQTT→UART) │
                                         └──────┬───────┘
                                                │ Serial2
                                         ┌──────┴───────┐
                                         │  Arduino Uno │
                                         │  (8 servos)  │
                                         └──────────────┘
```

## Wiring

### ESP32-CAM → Arduino Uno
```
ESP32 GPIO14 (TX2) ──→ [2.2kΩ] ──┬──→ Arduino pin 0 (RX)
                                  │
                                 [3.3kΩ]
                                  │
                                 GND
ESP32 GPIO15 (RX2) ←── Arduino pin 1 (TX)
ESP32 GND          ──── Arduino GND
```

### Arduino Uno → Servos
| Leg | Hip | Knee |
|-----|-----|------|
| RF  | 11  | 10   |
| RB  | 7   | 6    |
| LF  | 9   | 8    |
| LB  | 5   | 4    |

## MQTT Topics

| Topic | Direction | Payload |
|-------|-----------|---------|
| `robot/spider/cmd/direction` | Dashboard → Robot | `{"direction":"forward"}` |
| `robot/spider/cmd/speed` | Dashboard → Robot | `{"speed":50}` |
| `robot/spider/cmd/gait` | Dashboard → Robot | `{"gait":"tripod"}` |
| `robot/spider/cmd/rotation` | Dashboard → Robot | `{"angle":90}` |
| `robot/spider/cam/stream` | Robot → Dashboard | `{"frame":"<base64>","timestamp":123}` |
| `robot/spider/status` | Robot → Dashboard | `{"status":"online"}` |

## Commands

| # | Movement | Dashboard Trigger |
|---|----------|-------------------|
| 1 | Forward  | ArrowUp / FORWARD button |
| 2 | Backward | ArrowDown / BACKWARD button |
| 3 | Rotate Right | ArrowRight / ROTATE R button |
| 4 | Rotate Left  | ArrowLeft / ROTATE L button |
| 5 | Squat   | SQUAT button |
| 6 | Wiggle  | WIGGLE button |
| 0 | Rest    | Center dot / stop |

## Simulating in Wokwi

See `Wokwi/` directory for three test configurations:
- `Wokwi/` — Arduino + 8 servos (type commands 0-6 in serial monitor)
- `Wokwi/esp32-bridge/` — ESP32 MQTT reception test
- `Wokwi/full-system/` — ESP32 + Arduino end-to-end
