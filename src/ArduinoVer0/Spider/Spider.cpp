#include "Spider.h"

Spider::Spider() {
}

Spider::~Spider() {
  shutdown();
}

void Spider::setup() {
  Serial.begin(9600);
  
  for (int i = 0; i < 4; i++) {
    legs[i].attach(pinConfig[i][0], pinConfig[i][1]);
  }

  rest();
  
  Serial.println("Spider rahou nad w3333");
}

void Spider::shutdown() {
  for (int i = 0; i < 4; i++) {
    legs[i].detach();
  }
  Serial.println("Spider rgod b333");
}

void Spider::executeMovement(Movement& movement, int iterations) {
  Serial.print("rah yemchi: ");
  Serial.println(movement.name);
  
  for (int iter = 0; iter < iterations; iter++) {
    for (int frame = 0; frame < movement.frameCount; frame++) {
      for (int leg = 0; leg < 4; leg++) {
        int kneeAngle = movement.keyframes[leg][0][frame];
        int hipAngle = movement.keyframes[leg][1][frame];
        legs[leg].setPosition(hipAngle, kneeAngle);
      }
      
      delay(movement.delayMs);
    }
  }
}

void Spider::setLegPosition(int legIndex, int hipAngle, int kneeAngle) {
  if (legIndex >= 0 && legIndex < 4) {
    legs[legIndex].setPosition(hipAngle, kneeAngle);
  }
}

void Spider::setAllLegsPosition(int hipAngle[], int kneeAngle[]) {
  for (int i = 0; i < 4; i++) {
    legs[i].setPosition(hipAngle[i], kneeAngle[i]);
  }
}

void Spider::rest() {
  RestMovement rest;
  executeMovement(rest, 1);
}


void Spider::moveForward(int iterations) {
  ForwardMovement forward;
  executeMovement(forward, iterations);
}

void Spider::moveBackward(int iterations) {
  BackwardMovement backward;
  executeMovement(backward, iterations);
}
