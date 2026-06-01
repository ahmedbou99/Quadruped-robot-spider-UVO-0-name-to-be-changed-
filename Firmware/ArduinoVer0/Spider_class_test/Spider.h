#ifndef SPIDER_H
#define SPIDER_H

#include <Arduino.h>
#include "Leg.h"
#include "Movement.h"

// Leg 
#define RF 0 
#define RB 1 
#define LF 2
#define LB 3

class Spider {
private:
  Leg legs[4];
  
  // Pin configuration
  // Format: {hip_pin, knee_pin}
  uint8_t pinConfig[4][2] = {
    {11, 10},  // RF
    {7, 6},    // RB
    {9, 8},    // LF
    {5, 4}     // LB
  };
  
public:
  Spider();
  ~Spider();
  
  void setup();
  void shutdown();
  
  // Execute a movement pattern
  void executeMovement(Movement& movement, int iterations = 1);
  
  // Speed control (0-100, stored as percentage)
  int speedPercent = 50;
  void setSpeed(int pct);

  // Direct leg control
  void setLegPosition(int legIndex, int hipAngle, int kneeAngle);
  void setAllLegsPosition(int hipAngle[], int kneeAngle[]);

  // Utility
  void rest();
  
  // Direct movement calls
  void moveForward(int iterations = 1);
  void moveBackward(int iterations = 1);
  void RotateRight(int iterations = 1);
  void RotateLeft(int iterations = 1);
  //added squatting THE MOST UNDERRATED MOVE OAT
  void Squat(int iterations = 1);
  //added wiggling
  void Wiggle(int iterations = 1);
};

#endif
