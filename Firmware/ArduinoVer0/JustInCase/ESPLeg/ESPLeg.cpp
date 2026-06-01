//all length mesuremenets are in mm
//all angles are in degrees

#include "Arduino.h"
#include <ESP32Servo.h>
#include "ESPLeg.h"

ESPLeg::ESPLeg(){

};


void ESPLeg::constructLeg(uint8_t hp, uint8_t kp){ //to attach the pins of knee and hip to servos
  L1 = 50; //in mm
  L2 = 50; //in mm
  hipServo.attach(hp);
  kneeServo.attach(kp);
  kneePin = kp;
  hipPin = hp;
  isCalibrated = 0;
};

void ESPLeg::destroyLeg(){ //to separate the pins of knee and hip from servos
  hipServo.detach();
  kneeServo.detach();
};

void ESPLeg::calibrateLeg(){
  hipServo.write(90);
  kneeServo.write(180);
  isCalibrated = 1;
};



int* ESPLeg::goTo(float x, float y, float z){
  float ah;
  float ak;
  float b;
  float cosb;
  float L3;

  //limits in mm
  float yMin = 40;
  float zMin = -76;
  float yMax = 78;
  float zMax = -60;

  if(z >= zMax || y >= yMax){
    y = yMax;
    z = zMax;
  }
  else if (z <= zMin || y <= yMin){
    y = yMin;
    z = zMin;
  }

  L3 = sqrt((y*y) + (z*z));



  ah = atan2(y,x) * 180/PI;


  cosb = (L1*L1 + L2*L2 -L3*L3)/(2.0*L1*L2);
  b = acos(cosb); //knee angle


  ak =(b)*180/PI;







  Serial.println(ah);
  Serial.println(ak);


  int* angles = new int[2];
  angles[0] = ah;
  angles[1] = ak;

  return angles;


  //this method doesnt write the angles to the servos , it just gives the angles
  //for back legs of the robot , knee angles shall be reversed by substracting them from 180 degrees

};
