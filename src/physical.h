#ifndef PHYSICAL_H
#define PHYSICAL_H

const uint16_t kAcceleration = 12800;
const uint16_t kStepsPerRev = 16 * 200;

// initial homing speed (steps per second)
const uint16_t kHomingSpeed = 2 * kStepsPerRev;
// final homing speed (steps per second)
const uint16_t kFinalHomingSpeed = 0.5 * kStepsPerRev;
// backoff from homing switch (steps) after inital press
const uint16_t kHomeBackoff = 1 * kStepsPerRev;

#define dirPinStepper 32
#define enablePinStepper 25
#define stepPinStepper 33

#define homePin 26

#endif
