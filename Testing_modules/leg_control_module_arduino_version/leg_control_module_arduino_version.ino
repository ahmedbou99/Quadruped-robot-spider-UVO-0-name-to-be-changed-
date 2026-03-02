#include <Servo.h>
#include <iostream>
 float L1 = 5.0; //in cm
float L2 = 5.5; //in cm


class Leg{

Servo hipServo;
Servo kneeServo;
uint8_t hipPin;
uint8_t kneePin;
bool isCalibrated;

public : void constructLeg(uint8_t hp, uint8_t kp){ //to attach the pins of knee and hip to servos
hipServo.attach(hp);
kneeServo.attach(kp);
kneePin = kp;
hipPin = hp;
isCalibrated = 0;
};

public : void destroyLeg(){ //to separate the pins of knee and hip from servos
hipServo.detach();
kneeServo.detach();
};

public : void calibrateLeg(){
hipServo.write(90);
kneeServo.write(180);
isCalibrated = 1;
};



public : bool goTo(float x, float y, float z){
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
  Serial.printf("the hip angle  :%f\n",ah);
Serial.printf("the knee angle : %f\n",ak);
  hipServo.write((int)ah);
kneeServo.write((int)ak);
return 1;
}
else {
  Serial.println("coordinates out of reach");
  return 0;
};
};
};




#define HIP_PIN 17
#define KNEE_PIN 13
Leg leg;
double x,y,z;

void setup() {
   Serial.begin(115200);

leg.constructLeg(HIP_PIN,KNEE_PIN);
leg.calibrateLeg();

//Serial.println(leg.goTo(8, 8,0));









}

void loop() {
  

/* 
for (int i=0;i<=8;i+=1) {
  leg.goTo(i,8-i,i);
  delay(50);
}


for (int i=8;i>=0;i-=1) {
  leg.goTo(i,8-i,i);
  delay(50);
}
*/




}
