#include "Spider.h"

Spider spider;

void setup() {
  Serial.begin(9600);
  spider.setup();
  spider.rest();
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
      return;
    }

    int mv = line.toInt();

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
