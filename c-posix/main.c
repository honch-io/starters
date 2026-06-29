// Vending machine controller — a small simulation of the kind of long-lived
// daemon you'd run on the embedded-Linux brain of a connected vending machine.
//
// It owns a handful of product slots, a refrigerated cabinet, and a cash box,
// and it drives a simulated stream of real-world activity: customers buying
// drinks and snacks, the occasional failed vend, a technician restocking and
// collecting cash, and a periodic health heartbeat. Each action is a plain
// function that mutates controller state and logs what happened.
//
// There is no Honch SDK here — this is just the device. Run it and watch the
// machine go about its day.

#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#define SLOT_COUNT 6
#define FRIDGE_TARGET_C 4.0
#define FRIDGE_ALARM_C 8.0
#define HEARTBEAT_EVERY 8 // emit a health heartbeat every N simulation ticks

typedef enum { PAY_CARD, PAY_CASH, PAY_APP } payment_t;

typedef struct {
  const char *product;
  int price_cents;
  int stock;
  bool refrigerated;
} slot_t;

typedef struct {
  slot_t slots[SLOT_COUNT];
  double fridge_c;     // current cabinet temperature
  int sales_today;     // successful vends since start
  long cash_box_cents; // uncollected cash takings
  unsigned uptime_s;   // seconds since controller_start
} controller_t;

static volatile sig_atomic_t s_running = 1;

static void on_signal(int sig) {
  (void)sig;
  s_running = 0;
}

static const char *payment_name(payment_t p) {
  switch (p) {
    case PAY_CARD: return "card";
    case PAY_CASH: return "cash";
    case PAY_APP: return "app";
  }
  return "unknown";
}

// --- Device actions -------------------------------------------------------

static void controller_start(controller_t *c) {
  static const slot_t initial[SLOT_COUNT] = {
      {"Cola", 200, 8, true},        {"Sparkling Water", 180, 6, true},
      {"Iced Coffee", 320, 4, true}, {"Granola Bar", 250, 10, false},
      {"Crisps", 220, 7, false},     {"Chocolate", 240, 5, false},
  };
  for (int i = 0; i < SLOT_COUNT; i++) c->slots[i] = initial[i];
  c->fridge_c = FRIDGE_TARGET_C;
  c->sales_today = 0;
  c->cash_box_cents = 0;
  c->uptime_s = 0;
  printf("[start] controller online, %d slots stocked\n", SLOT_COUNT);
}

// A customer tries to buy from a slot. Succeeds only if it has stock; a card
// payment can still be declined. Returns true on a completed sale.
static bool vend_product(controller_t *c, int slot, payment_t pay) {
  slot_t *s = &c->slots[slot];
  if (s->stock <= 0) {
    printf("[vend_failed] slot %d (%s): out_of_stock\n", slot, s->product);
    return false;
  }
  if (pay == PAY_CARD && (rand() % 20) == 0) {
    printf("[vend_failed] slot %d (%s): payment_declined\n", slot, s->product);
    return false;
  }
  s->stock--;
  c->sales_today++;
  if (pay == PAY_CASH) c->cash_box_cents += s->price_cents;
  printf("[product_vended] slot %d: %s for %d cents via %s (%d left)\n", slot,
         s->product, s->price_cents, payment_name(pay), s->stock);
  return true;
}

// A technician opens the machine and refills a slot.
static void restocked(controller_t *c, int slot, int added) {
  c->slots[slot].stock += added;
  printf("[restocked] slot %d (%s): +%d -> %d\n", slot, c->slots[slot].product,
         added, c->slots[slot].stock);
}

static void service_access(const char *technician) {
  printf("[service_access] door opened by %s\n", technician);
}

// Empty the cash box during a service visit.
static void cash_collected(controller_t *c) {
  long amount = c->cash_box_cents;
  c->cash_box_cents = 0;
  printf("[cash_collected] %ld cents removed from box\n", amount);
}

// Sample the refrigerated cabinet and raise an alarm if it has drifted warm.
static void check_temperature(controller_t *c) {
  // Simple random walk around the target temperature.
  c->fridge_c += ((rand() % 30) - 14) / 10.0;
  if (c->fridge_c < 1.0) c->fridge_c = 1.0;
  if (c->fridge_c > FRIDGE_ALARM_C) {
    printf("[temperature_alarm] cabinet at %.1f C (threshold %.1f C)\n",
           c->fridge_c, FRIDGE_ALARM_C);
  }
}

static void heartbeat(const controller_t *c) {
  printf("[heartbeat] uptime=%us sales_today=%d cabinet=%.1fC\n", c->uptime_s,
         c->sales_today, c->fridge_c);
}

static void controller_stop(const controller_t *c) {
  printf("[stop] controller shutting down, %d sales today, %ld cents uncollected\n",
         c->sales_today, c->cash_box_cents);
}

// --- Simulation loop ------------------------------------------------------

int main(void) {
  signal(SIGINT, on_signal);
  signal(SIGTERM, on_signal);
  srand((unsigned)time(NULL));

  controller_t controller;
  controller_start(&controller);

  // Each tick is one "minute" of machine time, compressed to 300ms so the
  // demo is watchable. Ctrl-C to stop.
  for (unsigned tick = 0; s_running; tick++) {
    controller.uptime_s += 60;

    // Most ticks see a customer; sometimes nobody comes.
    if (rand() % 4 != 0) {
      int slot = rand() % SLOT_COUNT;
      payment_t pay = (payment_t)(rand() % 3);
      vend_product(&controller, slot, pay);
    }

    // A technician shows up roughly every 12 ticks to service the machine.
    if (tick > 0 && tick % 12 == 0) {
      service_access("field-tech");
      for (int i = 0; i < SLOT_COUNT; i++) {
        if (controller.slots[i].stock < 3) restocked(&controller, i, 8);
      }
      cash_collected(&controller);
    }

    check_temperature(&controller);

    if (tick % HEARTBEAT_EVERY == 0) heartbeat(&controller);

    usleep(300 * 1000);
  }

  controller_stop(&controller);
  return 0;
}
