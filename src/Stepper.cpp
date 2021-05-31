#include "Stepper.h"

#include "Arduino.h"
#include "esp_log.h"
const char *TAG = "stepper";

bool Stepper::init(FastAccelStepperEngine &engine, uint8_t pin_step, uint8_t pin_dir, uint8_t pin_enable, uint8_t pin_home) {
  _stepper = engine.stepperConnectToPin(pin_step);

  if (_stepper == NULL) {
    return false;
  }

  _stepper->setDirectionPin(pin_dir);
  _stepper->setEnablePin(pin_enable);
  _stepper->setAcceleration(kStepperDefaultAcceleration);

  pinMode(pin_home, INPUT_PULLUP);
  _pin_home = pin_home;

  esp_timer_create_args_t timer_args;
  memset(&timer_args, 0, sizeof(esp_timer_create_args_t));
  timer_args.arg = this;
  timer_args.callback = &homePinCheckWrapper;
  timer_args.name = "home check";

  esp_timer_handle_t timer;
  if (esp_timer_create(&timer_args, &timer) != ESP_OK) {
    return false;
  }
  esp_timer_start_periodic(timer, kStepperButtonCheckInterval);

  if (!xTaskCreate(Stepper::taskWrapper, "stepper", 4096, this, 10, &_task_handle)) {
    return false;
  }

  return true;
}

void Stepper::taskWrapper(void *arg) {
  Stepper *stepper = (Stepper *)arg;
  stepper->task();
}

void Stepper::task() {
  while (1) {
    ESP_LOGD(TAG, "Waiting for initial home press");
    waitHomeTrigger();
    ESP_LOGD(TAG, "Start homing");

    _stepper->enableOutputs();
    // stage 1: move towards home until home is triggered
    _stepper->setSpeedInHz(_param_homing_speed);
    _stepper->runBackward();

    ESP_LOGD(TAG, "Waiting for home press");
    waitHomeTrigger();
    _stepper->forceStopAndNewPosition(0);

    ESP_LOGD(TAG, "Start backoff");
    _stepper->move(_param_homing_backoff);
    ESP_LOGD(TAG, "Waiting for stepper to stop moving");
    while (_stepper->isRunning()) {
      vTaskDelay(1);
    }

    // stage 2: move towards home until home is triggered
    _stepper->setSpeedInHz(_param_final_homing_speed);
    _stepper->runBackward();

    waitHomeTrigger();
    _stepper->forceStopAndNewPosition(0);
    _stepper->disableOutputs();
  }
}

void IRAM_ATTR Stepper::homePinCheckWrapper(void *arg) {
  Stepper *stepper = (Stepper *)arg;
  stepper->homePinCheck();
};

void IRAM_ATTR Stepper::homePinCheck() {
  uint8_t cur_state = digitalRead(_pin_home);
  static bool reported = true;
  static uint8_t last_state = cur_state;
  static uint8_t stable_count = 0;

  if (cur_state != last_state) {
    stable_count = 0;
    reported = false;
  } else {
    stable_count++;
  }

  if (stable_count > kStepperButtonStableThreshold && !reported && cur_state == kStepperButtonTriggeredState) {
    xTaskNotify(_task_handle, cur_state, eSetValueWithOverwrite);
    reported = true;
  }

  last_state = cur_state;
};

void Stepper::waitHomeTrigger() {
  // clear any pending notifications
  xTaskNotifyWait(ULONG_MAX, ULONG_MAX, NULL, 0);
  // wait for next notification
  xTaskNotifyWait(0, 0, NULL, portMAX_DELAY);
}
