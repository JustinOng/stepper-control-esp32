#include <Arduino.h>
#include "FastAccelStepper.h"

#define dirPinStepper    32
#define enablePinStepper 25
#define stepPinStepper   33

#define homePin 26

const uint16_t kAcceleration = 12800;
const uint16_t kStepsPerRev = 16 * 200;

// initial homing speed (steps per second)
const uint16_t kHomingSpeed = 2 * kStepsPerRev;
// final homing speed (steps per second)
const uint16_t kFinalHomingSpeed = 0.5 * kStepsPerRev;
// backoff from homing switch (steps) after inital press
const uint16_t kHomeBackoff = 1 * kStepsPerRev;

FastAccelStepperEngine engine = FastAccelStepperEngine();
FastAccelStepper *stepper = NULL;

void setup() {
  engine.init();
  stepper = engine.stepperConnectToPin(stepPinStepper);
  if (stepper) {
    stepper->setDirectionPin(dirPinStepper);
    stepper->setEnablePin(enablePinStepper);
    stepper->setAutoEnable(false);

    stepper->setAcceleration(kAcceleration);
    // stepper->move(3200);
  }
  
  pinMode(homePin, INPUT_PULLUP);

  Serial.begin(115200);
}

void loop() {
  Serial.println("Waiting for initial home press");
  while (digitalRead(homePin)) {
    vTaskDelay(1);
  }
  Serial.println("Waiting for release");
  while (!digitalRead(homePin)) {
    vTaskDelay(1);
  }

  stepper->enableOutputs();
  // stage 1: home at kHomingSpeed until home is triggered
  stepper->setSpeedInHz(kHomingSpeed);
  stepper->runBackward();

  Serial.println("Waiting for home press");
  while (digitalRead(homePin)) {
    vTaskDelay(1);
  }
  stepper->stopMove();
  Serial.println("Start backoff");
  stepper->move(kHomeBackoff);
  Serial.println("Waiting for stepper to stop moving");
  while (stepper->isRunning()) {
    vTaskDelay(1);
  }

  // stage 2: home at kFinalHomingSpeed until home is triggered
  stepper->setSpeedInHz(kFinalHomingSpeed);
  stepper->runBackward();

  Serial.println("Waiting for home press");
  while (digitalRead(homePin)) {
    vTaskDelay(1);
  }
  stepper->stopMove();

  Serial.println("Disabled");
  stepper->disableOutputs();

  Serial.println("Done");

  while (1) {
    vTaskDelay(1000);
  }
}
