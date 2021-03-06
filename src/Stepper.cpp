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

  _command_queue = xQueueCreate(kStepperCommandQueueLength, sizeof(command_container_t));
  assert(_command_queue != NULL);

  if (!xTaskCreate(Stepper::taskWrapper, "stepper", 4096, this, 10, &_task_handle)) {
    return false;
  }

  return true;
}

void Stepper::taskWrapper(void *arg) {
  Stepper *stepper = (Stepper *)arg;
  stepper->task();
}

bool Stepper::home() {
  command_container_t command;
  command.command = COMMAND_HOME;
  return xQueueSend(_command_queue, &command, 0) == pdTRUE;
}

bool Stepper::moveTo(int32_t position) {
  command_container_t command;
  command.command = COMMAND_MOVE;
  command.data.int32 = position;
  return xQueueSend(_command_queue, &command, 0) == pdTRUE;
}

void Stepper::task() {
  while (1) {
    command_container_t command;
    xQueueReceive(_command_queue, &command, portMAX_DELAY);

    switch (command.command) {
      case COMMAND_HOME:
        ESP_LOGD(TAG, "Start homing");

        _stepper->enableOutputs();
        while (digitalRead(_pin_home) == kStepperButtonTriggeredState) {
          _stepper->setSpeedInHz(_param_homing_speed);
          _stepper->runForward();
        }
        _stepper->forceStopAndNewPosition(0);

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

        ESP_LOGD(TAG, "Waiting for home press");
        waitHomeTrigger();
        _stepper->forceStopAndNewPosition(0);
        _stepper->disableOutputs();

        ESP_LOGD(TAG, "Homing done");
        break;
      case COMMAND_MOVE:
        _stepper->setSpeedInHz(_param_move_speed);
        _stepper->enableOutputs();
        _stepper->moveTo(command.data.int32);
        while (_stepper->isRunning()) {
          vTaskDelay(pdMS_TO_TICKS(100));
        }
        _stepper->disableOutputs();
        break;
      default:
        ESP_LOGW(TAG, "Unhandled command");
        break;
    }
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
