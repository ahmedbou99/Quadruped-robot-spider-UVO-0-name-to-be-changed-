#include "Spider.h"
#include <SoftwareSerial.h>

Spider spider;
SoftwareSerial softSerial(2, 3);  // RX=pin2 ← ESP32 TX, TX=pin3 (unused)

void setup() {
  Serial.begin(9600);         // USB debug
  softSerial.begin(9600);     // ESP32 data
  spider.setup();
  spider.rest();
  Serial.println("Arduino ready");
}

void loop() {
  if (softSerial.available() > 0) {
    String line = softSerial.readStringUntil('\n');
    line.trim();
    if (line.length() == 0) return;

    Serial.print("RX: ");
    Serial.println(line);

    if (line[0] == 'S') {
      int spd = line.substring(1).toInt();
      spider.setSpeed(spd);
      Serial.print("Speed: ");
      Serial.println(spd);
      return;
    }

    int mv = line.toInt();

    switch (mv) {
      case 1:
        Serial.println("Forward");
        spider.moveForward();
        break;
      case 2:
        Serial.println("Backward");
        spider.moveBackward();
        break;
      case 3:
        Serial.println("Rotate R");
        spider.RotateRight();
        break;
      case 4:
        Serial.println("Rotate L");
        spider.RotateLeft();
        break;
      case 5:
        Serial.println("Squat");
        spider.Squat();
        break;
      case 6:
        Serial.println("Wiggle");
        spider.Wiggle();
        break;
      case 0:
        Serial.println("Rest");
        spider.rest();
        break;
    }
  }
  delay(10);
}
