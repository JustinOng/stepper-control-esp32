#include <Arduino.h>

#include "FastAccelStepper.h"
#include "Stepper.h"
#include "physical.h"

FastAccelStepperEngine engine = FastAccelStepperEngine();
Stepper stepper;

static const char *TAG = "main";

void setup() {
  engine.init();
  ESP_LOGI(TAG, "starting");
  if (!stepper.init(engine, stepPinStepper, dirPinStepper, enablePinStepper, homePin)) {
    ESP_LOGW(TAG, "Init fail");
  }
  stepper.setAcceleration(kAcceleration);

  stepper.home();
}

void loop() {
}
