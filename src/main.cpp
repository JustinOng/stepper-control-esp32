#include <Arduino.h>
#include "FastAccelStepper.h"
#include "Stepper.h"
#include "physical.h"

FastAccelStepperEngine engine = FastAccelStepperEngine();
Stepper stepper;

void setup() {
  engine.init();

  Serial.begin(115200);
  if (!stepper.init(engine, stepPinStepper, dirPinStepper, enablePinStepper, homePin)) {
    Serial.println("Init fail");
  }
  stepper.setAcceleration(kAcceleration);
}

void loop() {
}
