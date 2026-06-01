#include "Spider.h"
Spider spider;

void blink(int times) {
  for (int i = 0; i < times; i++) {
    digitalWrite(LED_BUILTIN, LOW); delay(150);
    digitalWrite(LED_BUILTIN, HIGH); delay(150);
  }
  delay(500);
}

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);  // LED ON during setup
  Serial.begin(9600);
  spider.setup();
  spider.rest();
  digitalWrite(LED_BUILTIN, LOW);   // LED OFF = ready
}

void loop() {
  if (Serial.available() > 0) {
    String line = Serial.readStringUntil('\n');
    line.trim();
    if (line.length() == 0) return;

    if (line[0] == 'S') {
      int spd = line.substring(1).toInt();
      spider.setSpeed(spd);
      blink(1);  // 1 blink = speed received
      return;
    }

    int mv = line.toInt();
    blink(mv);   // blink count = command number (0 blinks = rest)

    switch (mv) {
      case 0: spider.rest(); break;
      case 1: spider.moveForward(); break;
      case 2: spider.moveBackward(); break;
      case 3: spider.RotateRight(); break;
      case 4: spider.RotateLeft(); break;
      case 5: spider.Squat(); break;
      case 6: spider.Wiggle(); break;
    }
  }
  delay(10);
}
