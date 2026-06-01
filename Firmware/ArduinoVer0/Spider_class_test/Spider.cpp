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

void Spider::setSpeed(int pct) {
  speedPercent = constrain(pct, 0, 100);
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
      
      int adjDelay = movement.delayMs * (200 - speedPercent * 3 / 2) / 100;
      if (adjDelay < 1) adjDelay = 1;
      delay(adjDelay);
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
  legs[0].setPosition(45,150);
  legs[1].setPosition(135,20);
  legs[2].setPosition(135,150);
  legs[3].setPosition(45,20);

}


void Spider::moveForward(int iterations) {
  ForwardMovement forward;
  executeMovement(forward, iterations);
}

void Spider::moveBackward(int iterations) {
  BackwardMovement backward;
  executeMovement(backward, iterations);
}

void Spider::RotateRight(int iterations) {
  RotateRightMovement rotateright;
  executeMovement(rotateright, iterations);
}

void Spider::RotateLeft(int iterations) {
  RotateLeftMovement rotateleft;
  executeMovement(rotateleft, iterations);
}
//added squatting THE MOST UNDERRATED MOVE OAT
void Spider :: Squat(int iterations){
  SquatMovement squat;
  executeMovement(squat,iterations);
}


//added wiggling
void Spider :: Wiggle(int iterations){
  WiggleMovement wiggle;
  executeMovement(wiggle,iterations);
}
