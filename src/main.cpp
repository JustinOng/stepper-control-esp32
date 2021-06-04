#include <Arduino.h>

#include "FastAccelStepper.h"
#include "Stepper.h"
#include "argtable3/argtable3.h"
#include "esp_console.h"
#include "physical.h"

FastAccelStepperEngine engine = FastAccelStepperEngine();
Stepper stepper;

static const char *TAG = "main";
static const uint8_t kSerialBufferSize = 32;

static struct {
  struct arg_int *channel;
  struct arg_end *end;
} args_home;

static int do_home_cmd(int argc, char **argv) {
  int nerrors = arg_parse(argc, argv, (void **)&args_home);
  if (nerrors != 0) {
    arg_print_errors(stderr, args_home.end, argv[0]);
    return 0;
  }

  ESP_LOGI(TAG, "Homing channel=%d", args_home.channel->ival[0]);
  stepper.home();

  return 0;
}

void setup() {
  engine.init();
  Serial.begin(115200);
  ESP_LOGI(TAG, "starting");
  if (!stepper.init(engine, stepPinStepper, dirPinStepper, enablePinStepper, homePin)) {
    ESP_LOGW(TAG, "Init fail");
  }
  stepper.setAcceleration(kAcceleration);

  esp_console_config_t console_config;
  memset(&console_config, 0, sizeof(console_config));
  console_config.max_cmdline_length = 32;
  console_config.max_cmdline_args = 3;
  esp_console_init(&console_config);

  args_home.channel = arg_int1(NULL, NULL, "<channel>", "channel to home");
  args_home.end = arg_end(1);

  esp_console_cmd_t home_cmd;
  memset(&home_cmd, 0, sizeof(home_cmd));

  home_cmd.command = "home";
  home_cmd.help = NULL;
  home_cmd.hint = NULL;
  home_cmd.func = &do_home_cmd;
  home_cmd.argtable = &args_home;

  ESP_ERROR_CHECK(esp_console_cmd_register(&home_cmd));
}

void loop() {
  static char buf[kSerialBufferSize];
  static uint8_t buf_size = 0;

  while (Serial.available()) {
    char incoming = Serial.read();

    if ((incoming == '\r' || incoming == '\n')) {
      if (buf_size > 0) {
        int ret = -1;
        esp_console_run(buf, &ret);
        ESP_LOGI(TAG, "ret=%d", ret);

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
