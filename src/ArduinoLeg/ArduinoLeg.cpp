#include "Arduino.h"
#include <Servo.h>
#include "ArduinoLeg.h"

ArduinoLeg::ArduinoLeg(){

};


 void ArduinoLeg::constructLeg(uint8_t hp, uint8_t kp){ //to attach the pins of knee and hip to servos
L1 = 5.0;
L2 = 5.5;
hipServo.attach(hp);
kneeServo.attach(kp);
kneePin = kp;
hipPin = hp;
isCalibrated = 0;
};

void ArduinoLeg::destroyLeg(){ //to separate the pins of knee and hip from servos
hipServo.detach();
kneeServo.detach();
};

void ArduinoLeg::calibrateLeg(){
hipServo.write(90);
kneeServo.write(180);
isCalibrated = 1;
};



 bool ArduinoLeg::goTo(float x, float y, float z){
float ak = 180;
float ah;
//float g;
float b;
float cosb;
float L3;


L3 = sqrt((y*y) + (z*z) + (x*x));


ah = atan2(y,x) * 180/PI;


cosb = (L1*L1 + L2*L2 -L3*L3)/(2.0*L1*L2);
b = acos(cosb); //knee angle


ak = b*180/PI;

ak = constrain(ak, 90,180);
ah = constrain(ah, 0,180);








if(L3 <= L1 +L2){
  Serial.println(ah);
Serial.println(ak);
  hipServo.write((int)ah);
kneeServo.write((int)ak);
return 1;
}
else {
  Serial.println("coordinates out of reach");
  return 0;
};
};
