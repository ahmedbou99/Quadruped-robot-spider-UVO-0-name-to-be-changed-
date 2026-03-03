#ifndef ArduinoLeg_h
#define ArduinoLeg_h

#include "Arduino.h"
#include <Servo.h>

class ArduinoLeg{
  public :
  ArduinoLeg();
  Servo hipServo;
Servo kneeServo;
uint8_t hipPin;
uint8_t kneePin;
bool isCalibrated;
void constructLeg(uint8_t hp, uint8_t kp);
void destroyLeg();
void calibrateLeg();
bool goTo(float x, float y, float z);
};


#endif