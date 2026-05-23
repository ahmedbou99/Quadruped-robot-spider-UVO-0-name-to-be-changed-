#ifndef MOVEMENT_H
#define MOVEMENT_H

#include <Arduino.h>
#include "Leg.h"


struct Movement {
  const char* name;
  int frameCount;
  int delayMs;

  int keyframes[4][2][16];
  
  Movement(const char* n, int frames, int delay) 
    : name(n), frameCount(frames), delayMs(delay) {}
};

// Forward
class ForwardMovement : public Movement {
public:
  ForwardMovement();
};

// Backward
class BackwardMovement : public Movement {
public:
  BackwardMovement();
};

// RotateRight
class RotateRightMovement : public Movement {
public:
  RotateRightMovement();
};

// RotateLeft
class RotateLeftMovement : public Movement {
public:
  RotateLeftMovement();
};

//Squat
//added squatting THE MOST UNDERRATED MOVE OAT

class SquatMovement : public Movement {
public:
  SquatMovement();
};

//Wiggle
//added wiggling (i guess spiders do that to look intimidating to predators :)  )
class WiggleMovement : public Movement {
public:
   WiggleMovement();
};


#endif
