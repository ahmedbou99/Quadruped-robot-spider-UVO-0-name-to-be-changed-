#ifndef ESPLeg_h
#define ESPLeg_h

#include "Arduino.h"
#include <ESP32Servo.h>

class ESPLeg{
public :
  ESPLeg();
  Servo hipServo;
  Servo kneeServo;
  uint8_t hipPin;
  uint8_t kneePin;
  float L1;
  float L2;
  bool isCalibrated;
  void constructLeg(uint8_t hp, uint8_t kp);
  void destroyLeg();
  void calibrateLeg();
  int* goTo(float x, float y, float z);
};


#endif

