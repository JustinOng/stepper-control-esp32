#include <Arduino.h>

#include "FastAccelStepper.h"
#include "Stepper.h"
#include "physical.h"

FastAccelStepperEngine engine = FastAccelStepperEngine();
Stepper stepper;

static const char *TAG = "main";
static const uint8_t kSerialBufferSize = 32;

void setup() {
  engine.init();
  Serial.begin(115200);
  ESP_LOGI(TAG, "starting");
  if (!stepper.init(engine, stepPinStepper, dirPinStepper, enablePinStepper, homePin)) {
    ESP_LOGW(TAG, "Init fail");
  }
  stepper.setAcceleration(kAcceleration);
}

void loop() {
  static char buf[kSerialBufferSize];
  static uint8_t buf_size = 0;

  while (Serial.available()) {
    char incoming = Serial.read();

    if ((incoming == '\r' || incoming == '\n')) {
      if (buf_size > 0) {
        int channel;
        float position;

        if (sscanf(buf, "home %d", &channel) == 1) {
          ESP_LOGI(TAG, "Homing channel=%d", channel);
          stepper.home();
        } else if (sscanf(buf, "move %d %f", &channel, &position) == 2) {
          if (position < 0 || position > kStepperMaxPositionMm) {
            ESP_LOGI(TAG, "Invalid position given");
          } else {
            int32_t target_steps = position * kStepperStepsPerMm;

            ESP_LOGI(TAG, "Moving channel=%d to %ld", channel, target_steps);
            stepper.moveTo(target_steps);
          }
        }

        buf_size = 0;
        memset(&buf, 0, kSerialBufferSize);
      }

      Serial.println();
    } else {
      Serial.write(incoming);
      buf[buf_size++] = incoming;
    }
  }
}
