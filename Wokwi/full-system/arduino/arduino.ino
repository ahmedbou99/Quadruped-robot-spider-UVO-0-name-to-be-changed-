#include "Spider.h"
 Spider spider;

void setup() {
Serial.begin(9600);
spider.setup();
spider.rest();
  
  
  


}

void loop() {
  if (Serial.available() > 0) {
    int mv = Serial.parseInt();

    switch (mv) {
      case 1:
        Serial.println("Moving Forward");
        spider.moveForward();
        break;
      case 2:
        Serial.println("Moving Backward");
        spider.moveBackward();
        break;
      case 3:
        Serial.println("Rotating Right");
        spider.RotateRight();
        break;
      case 4:
        Serial.println("Rotating Left");
        spider.RotateLeft();
        break;
      case 5:
        Serial.println("Squatting");
        spider.Squat();
        break;
      case 6:
        Serial.println("Wiggling");
        spider.Wiggle();
        break;
      case 0:
        Serial.println("Resting");
        spider.rest();
        break;
    }
  }
  delay(100);
}








