#include "Spider.h"

Spider spider;

void blink(byte n) {
  for (byte i = 0; i < n; i++) {
    digitalWrite(LED_BUILTIN, LOW); delay(100);
    digitalWrite(LED_BUILTIN, HIGH); delay(100);
  }
  delay(400);
}

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);
  Serial.begin(9600);
  spider.setup();
  spider.rest();
  digitalWrite(LED_BUILTIN, LOW);  // LED off = ready
}

void loop() {
  if (Serial.available() > 0) {
    String line = Serial.readStringUntil('\n');
    line.trim();
    if (line.length() == 0) return;
    if (line[0] != 'S' && !isdigit(line[0])) return;

    if (line[0] == 'S') {
      int spd = line.substring(1).toInt();
      spider.setSpeed(spd);
      blink(1);  // 1 blink = speed
      return;
    }

    int mv = line.toInt();
    blink(mv == 0 ? 1 : mv + 1);  // 1 blink=rest, 2=forward, 3=backward, etc.

    switch (mv) {
      case 1: spider.moveForward(); break;
      case 2: spider.moveBackward(); break;
      case 3: spider.RotateRight(); break;
      case 4: spider.RotateLeft(); break;
      case 5: spider.Squat(); break;
      case 6: spider.Wiggle(); break;
      case 0: spider.rest(); break;
    }
  }
  delay(10);
}
