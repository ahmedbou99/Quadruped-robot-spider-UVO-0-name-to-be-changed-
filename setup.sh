#!/bin/bash
# Quadruped Spider Robot — Setup Script
# Run once after cloning:  chmod +x setup.sh && ./setup.sh

set -e
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
cd "$SCRIPT_DIR"

echo "============================================"
echo " Quadruped Spider Robot — Setup"
echo "============================================"

# ── 1. TLS certificates ──────────────────────
echo ""
echo "[1/4] TLS certificates..."
cd mosquitto/certs

if [ ! -f ca.key ] || [ ! -f server.key ]; then
  openssl req -x509 -newkey rsa:2048 -keyout ca.key -out ca.crt -days 3650 -nodes -subj "/CN=mqtt.local" 2>/dev/null
  openssl req -newkey rsa:2048 -keyout server.key -nodes -out server.csr -subj "/CN=mqtt.local" 2>/dev/null
  openssl x509 -req -in server.csr -CA ca.crt -CAkey ca.key -CAcreateserial -out server.crt -days 3650 2>/dev/null
  rm -f server.csr
  chmod 600 *.key
  echo "  ✓ Certificates generated"
else
  echo "  ✓ Certificates already exist"
fi
cd "$SCRIPT_DIR"

# ── 2. MQTT password file ────────────────────
echo ""
echo "[2/4] MQTT password file..."
cd mosquitto
if [ ! -f passwd ]; then
  touch passwd
  mosquitto_passwd -b passwd spider_robot spider123 2>/dev/null
  mosquitto_passwd -b passwd dashboard    dash123   2>/dev/null
  mosquitto_passwd -b passwd esp32       esp32     2>/dev/null
  chmod 600 passwd
  echo "  ✓ Passwords created (spider_robot, dashboard, esp32)"
else
  echo "  ✓ Password file already exists"
fi
cd "$SCRIPT_DIR"

# ── 3. Mosquitto config ─────────────────────┬─
echo ""
echo "[3/4] Mosquitto config..."
CERT_DIR="$SCRIPT_DIR/mosquitto/certs"
PASSWD="$SCRIPT_DIR/mosquitto/passwd"
ACL="$SCRIPT_DIR/mosquitto/acl.conf"

cat > mosquitto/mosquitto-local.conf << MOSQUITTO
per_listener_settings true

# ── MQTT plain ──
listener 1883
allow_anonymous false
password_file $PASSWD
acl_file $ACL

# ── MQTT + TLS ──
listener 8883
cafile $CERT_DIR/ca.crt
certfile $CERT_DIR/server.crt
keyfile $CERT_DIR/server.key
tls_version tlsv1.2
allow_anonymous false
password_file $PASSWD
acl_file $ACL

# ── WebSocket + TLS ──
listener 9001
protocol websockets
cafile $CERT_DIR/ca.crt
certfile $CERT_DIR/server.crt
keyfile $CERT_DIR/server.key
tls_version tlsv1.2
allow_anonymous false
password_file $PASSWD
acl_file $ACL

# ── WebSocket plain (local dev) ──
listener 9002
protocol websockets
allow_anonymous false
password_file $PASSWD
acl_file $ACL

log_dest stdout
log_type error
log_type warning
log_type notice
log_timestamp true
MOSQUITTO
echo "  ✓ Config written with paths for this machine"

# ── 4. /etc/hosts check ─────────────────────
echo ""
echo "[4/4] Hosts file..."
if grep -q "mqtt.local" /etc/hosts 2>/dev/null; then
  echo "  ✓ mqtt.local already in /etc/hosts"
else
  echo "  ! Add this to /etc/hosts (requires sudo):"
  echo "    127.0.0.1 mqtt.local"
  echo "    sudo sh -c 'echo \"127.0.0.1 mqtt.local\" >> /etc/hosts'"
fi

# ── Done ────────────────────────────────────
echo ""
echo "============================================"
echo " Setup complete."
echo ""
echo "  Start broker:  mosquitto -c mosquitto/mosquitto-local.conf -d"
echo "  Web dashboard: cd Web && npm install && npm run dev"
echo "  Flash ESP32:   Firmware/ESP32_MQTT_Bridge/ESP32CAM_Bridge.ino"
echo "  Flash Arduino: Firmware/ArduinoVer0/Spider_class_test/Spider_class_test.ino"
echo "============================================"
