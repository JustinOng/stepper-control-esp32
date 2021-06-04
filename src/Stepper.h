#ifndef STEPPER_H
#define STEPPER_H

#include <stdint.h>

#include "FastAccelStepper.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"

const uint16_t kStepperButtonCheckInterval = 10000;
// how many kButtonCheckInterval must the state be the same to be considered stable
const uint8_t kStepperButtonStableThreshold = 4;
const uint8_t kStepperButtonTriggeredState = 0;

const uint16_t kStepperDefaultStepsPerRev = 16 * 200;
const uint16_t kStepperDefaultAcceleration = 12800;
const uint16_t kStepperDefaultHomingSpeed = 2 * kStepperDefaultStepsPerRev;
const uint16_t kStepperDefaultFinalHomingSpeed = 0.5 * kStepperDefaultStepsPerRev;
const uint16_t kStepperDefaultHomingBackoff = 1 * kStepperDefaultStepsPerRev;

// max number of queued commands
const uint8_t kStepperCommandQueueLength = 4;

class Stepper {
 public:
  typedef enum {
    COMMAND_HOME
  } command_t;

  typedef struct {
    command_t command;
    union {
      int32_t int32;
    } data;
  } command_container_t;

  bool init(FastAccelStepperEngine &engine, uint8_t pin_step, uint8_t pin_dir, uint8_t pin_enable, uint8_t pin_home);
  static void homePinCheckWrapper(void *arg);
  static void taskWrapper(void *arg);
  void task();
  void homePinCheck();

  bool home();
  bool moveTo(int32_t position);

  void setAcceleration(uint16_t accel) {
    _stepper->setAcceleration(accel);
  }

  void setHomingSpeed(uint16_t initial_speed, uint16_t final_speed) {
    _param_homing_speed = initial_speed;
    _param_final_homing_speed = final_speed;
  }

  void setHomingBackoff(uint16_t backoff) {
    _param_homing_backoff = backoff;
  }

 private:
  FastAccelStepper *_stepper = NULL;
  uint8_t _pin_home;
  TaskHandle_t _task_handle = NULL;

  QueueHandle_t _command_queue = NULL;

  uint16_t _param_homing_speed = kStepperDefaultHomingSpeed;
  uint16_t _param_final_homing_speed = kStepperDefaultFinalHomingSpeed;
  uint16_t _param_homing_backoff = kStepperDefaultHomingBackoff;

  void waitHomeTrigger();
};

#endif
