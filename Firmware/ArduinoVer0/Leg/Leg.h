#ifndef LEG_H
#define LEG_H

#include <Arduino.h>
#include <Servo.h>

class Leg {
private:
  Servo hipServo;
  Servo kneeServo;
  uint8_t hipPin;
  uint8_t kneePin;
  
public:
  Leg();
  ~Leg();
  
  void attach(uint8_t hp, uint8_t kp);
  void detach();
  void setHip(int angle);
  void setKnee(int angle);
  void setPosition(int hipAngle, int kneeAngle);
  
  int getHipAngle();
  int getKneeAngle();
};

#endif
