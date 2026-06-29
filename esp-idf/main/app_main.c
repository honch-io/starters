// Connected espresso machine — a simulation of the firmware for a Wi-Fi
// connected coffee machine running on an ESP32.
//
// It models the consumables and state a real machine tracks (water tank, bean
// hopper, boiler temperature, descale counter) and runs a "barista" loop that
// pulls shots throughout the day: starting and completing brews, frothing milk
// for milk drinks, warning when the tank or beans run low, asking to be
// descaled, running cleaning cycles, and occasionally hitting a hardware fault.
//
// There is no Honch SDK here — this is just the firmware. It builds and runs as
// a normal ESP-IDF app; the simulated machine drives itself on a FreeRTOS task.

#include <stdbool.h>
#include <stdint.h>

#include "esp_log.h"
#include "esp_random.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "espresso";

#define TANK_LOW_PERCENT 15
#define BEANS_LOW_PERCENT 15
#define DESCALE_AFTER_CUPS 150
#define HEARTBEAT_EVERY 10 // emit a health heartbeat every N brews

typedef struct {
  const char *name;
  int size_ml;
  bool with_milk;
} drink_t;

static const drink_t MENU[] = {
    {"espresso", 30, false}, {"lungo", 90, false},  {"americano", 180, false},
    {"cappuccino", 180, true}, {"latte", 240, true}, {"flat_white", 160, true},
};
#define MENU_COUNT (sizeof(MENU) / sizeof(MENU[0]))

typedef struct {
  int water_tank_percent;
  int bean_hopper_percent;
  double boiler_temp_c;
  uint32_t cups_today;
  uint32_t cups_since_descale;
  uint32_t uptime_s;
} machine_t;

static machine_t s_machine;

static uint32_t rnd(uint32_t modulo) { return esp_random() % modulo; }

// --- Device actions -------------------------------------------------------

static void machine_power_on(void) {
  s_machine.water_tank_percent = 100;
  s_machine.bean_hopper_percent = 100;
  s_machine.boiler_temp_c = 92.0;
  s_machine.cups_today = 0;
  s_machine.cups_since_descale = 0;
  s_machine.uptime_s = 0;
  ESP_LOGI(TAG, "machine_power_on: heating to brew temperature");
}

static void brew_started(const drink_t *drink, int grind) {
  ESP_LOGI(TAG, "brew_started: %s %dml grind=%d", drink->name, drink->size_ml,
           grind);
}

static void brew_completed(const drink_t *drink, double water_temp_c,
                           int shot_seconds) {
  s_machine.cups_today++;
  s_machine.cups_since_descale++;
  s_machine.water_tank_percent -= 3;
  s_machine.bean_hopper_percent -= 2;
  ESP_LOGI(TAG, "brew_completed: %s %dml water=%.1fC shot=%ds", drink->name,
           drink->size_ml, water_temp_c, shot_seconds);
}

static void milk_frothed(int seconds, double temp_c) {
  ESP_LOGI(TAG, "milk_frothed: %ds to %.1fC", seconds, temp_c);
}

static void water_tank_low(int percent) {
  ESP_LOGW(TAG, "water_tank_low: %d%% remaining", percent);
}

static void beans_low(int percent) {
  ESP_LOGW(TAG, "beans_low: %d%% remaining", percent);
}

static void descale_due(uint32_t cups_since_descale) {
  ESP_LOGW(TAG, "descale_due: %u cups since last descale", cups_since_descale);
}

static void cleaning_cycle_run(const char *kind) {
  ESP_LOGI(TAG, "cleaning_cycle_run: %s", kind);
}

// A hardware fault during a brew (pump/heater/grinder). Logged as an error so
// it stands out from normal operation.
static void brew_error(int code, const char *component) {
  ESP_LOGE(TAG, "brew_error: code=%d component=%s", code, component);
}

static void heartbeat(void) {
  ESP_LOGI(TAG, "heartbeat: uptime=%us cups_today=%u boiler=%.1fC",
           s_machine.uptime_s, s_machine.cups_today, s_machine.boiler_temp_c);
}

// Refill consumables (a person topping up the machine) so the demo runs
// indefinitely without drying out.
static void refill_if_empty(void) {
  if (s_machine.water_tank_percent <= 5) {
    s_machine.water_tank_percent = 100;
    ESP_LOGI(TAG, "water tank refilled");
  }
  if (s_machine.bean_hopper_percent <= 5) {
    s_machine.bean_hopper_percent = 100;
    ESP_LOGI(TAG, "bean hopper refilled");
  }
}

// --- Simulation -----------------------------------------------------------

// Pull one drink from start to finish, including the random chance of a fault.
static void pull_one_shot(void) {
  const drink_t *drink = &MENU[rnd(MENU_COUNT)];
  int grind = 3 + (int)rnd(6); // grind setting 3..8

  brew_started(drink, grind);
  vTaskDelay(pdMS_TO_TICKS(400));

  // ~1 brew in 40 hits a hardware fault and aborts.
  if (rnd(40) == 0) {
    static const char *parts[] = {"pump", "heater", "grinder"};
    brew_error(500 + (int)rnd(3), parts[rnd(3)]);
    return;
  }

  double water_temp_c = 90.0 + (double)rnd(40) / 10.0; // 90.0..93.9 C
  int shot_seconds = 22 + (int)rnd(10);                // 22..31 s
  brew_completed(drink, water_temp_c, shot_seconds);

  if (drink->with_milk) {
    milk_frothed(8 + (int)rnd(6), 60.0 + (double)rnd(40) / 10.0);
  }
}

static void barista_task(void *arg) {
  (void)arg;
  for (uint32_t brew = 0;; brew++) {
    s_machine.uptime_s += 90;

    pull_one_shot();

    if (s_machine.water_tank_percent <= TANK_LOW_PERCENT) {
      water_tank_low(s_machine.water_tank_percent);
    }
    if (s_machine.bean_hopper_percent <= BEANS_LOW_PERCENT) {
      beans_low(s_machine.bean_hopper_percent);
    }
    if (s_machine.cups_since_descale >= DESCALE_AFTER_CUPS) {
      descale_due(s_machine.cups_since_descale);
      cleaning_cycle_run("descale");
      s_machine.cups_since_descale = 0;
    }

    // A quick rinse cycle every dozen cups.
    if (brew > 0 && brew % 12 == 0) cleaning_cycle_run("rinse");

    if (brew % HEARTBEAT_EVERY == 0) heartbeat();

    refill_if_empty();

    vTaskDelay(pdMS_TO_TICKS(1500));
  }
}

void app_main(void) {
  machine_power_on();
  // The machine runs itself on its own task; app_main returns and ESP-IDF
  // keeps the scheduler running.
  xTaskCreate(barista_task, "barista", 4096, NULL, 5, NULL);
}
