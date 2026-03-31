#include "Leg.h"

Leg::Leg() {
}

Leg::~Leg() {
  detach();
}

void Leg::attach(uint8_t hp, uint8_t kp) {
  hipPin = hp;
  kneePin = kp;
  hipServo.attach(hipPin);
  kneeServo.attach(kneePin);
}

void Leg::detach() {
  hipServo.detach();
  kneeServo.detach();
}

void Leg::setHip(int angle) {
  angle = constrain(angle, 0, 180);
  hipServo.write(angle);
}

void Leg::setKnee(int angle) {
  angle = constrain(angle, 0, 180);
  kneeServo.write(angle);
}

void Leg::setPosition(int hipAngle, int kneeAngle) {
  setHip(hipAngle);
  setKnee(kneeAngle);
}

int Leg::getHipAngle() {
  return hipServo.read();
}

int Leg::getKneeAngle() {
  return kneeServo.read();
}
