# Cold/hot water dispenser — a simulation of the firmware for a connected water
# cooler running MicroPython on a microcontroller (ESP32, RP2040, ...).
#
# It models a chilled tank and a heated tank, a filter that wears out with use,
# and a child lock, then runs a loop simulating a day at the office water
# cooler: people dispensing cold, hot, or ambient water, tanks being refilled,
# the filter wearing low, the child lock being toggled, periodic temperature
# readings, the odd hardware fault, and a health heartbeat.
#
# There is no Honch SDK here — this is just the firmware. Flash it to a board and
# watch the dispenser run.

import machine
import random
import time

COLD_TARGET_C = 6.0
HOT_TARGET_C = 92.0
FILTER_LOW_PERCENT = 15
HEARTBEAT_EVERY = 10  # emit a health heartbeat every N dispenses


class Dispenser:
    def __init__(self):
        self.cold_c = COLD_TARGET_C
        self.hot_c = HOT_TARGET_C
        self.filter_life_percent = 100
        self.dispenses_today = 0
        self.child_lock = False
        self.uptime_s = 0

    # --- Device actions ---------------------------------------------------

    def boot(self):
        # machine.reset_cause() tells us why the board came up (power-on,
        # watchdog, deep-sleep wake, ...).
        print("dispenser_boot: reset_cause={}".format(machine.reset_cause()))

    def dispense_water(self, temperature):
        if self.child_lock and temperature == "hot":
            # Hot water is locked out for safety while the child lock is on.
            self.fault(1, "hot dispense blocked by child lock")
            return
        start = time.ticks_ms()
        volume_ml = random.choice((150, 250, 330, 500))
        # Larger pours take longer; ~4ml/ms is just a plausible flow rate.
        time.sleep_ms(volume_ml // 4)
        duration_ms = time.ticks_diff(time.ticks_ms(), start)

        self.dispenses_today += 1
        self.filter_life_percent = max(0, self.filter_life_percent - 1)
        if temperature == "cold":
            self.cold_c += 0.4  # tank warms slightly after a pour
        elif temperature == "hot":
            self.hot_c -= 1.0
        print(
            "water_dispensed: {} {}ml in {}ms".format(
                temperature, volume_ml, duration_ms
            )
        )

    def tank_refilled(self, tank):
        if tank == "cold":
            self.cold_c = COLD_TARGET_C
        else:
            self.hot_c = HOT_TARGET_C
        print("tank_refilled: {}".format(tank))

    def filter_low(self):
        print("filter_low: {}% life remaining".format(self.filter_life_percent))

    def child_lock_toggled(self, enabled):
        self.child_lock = enabled
        print("child_lock_toggled: enabled={}".format(enabled))

    def temperature_reading(self):
        print(
            "temperature_reading: cold={:.1f}C hot={:.1f}C".format(
                self.cold_c, self.hot_c
            )
        )

    def fault(self, code, message):
        print("fault: code={} {}".format(code, message))

    def heartbeat(self):
        print(
            "heartbeat: uptime={}s dispenses_today={}".format(
                self.uptime_s, self.dispenses_today
            )
        )


def run():
    dispenser = Dispenser()
    dispenser.boot()

    beat = 0
    while True:
        dispenser.uptime_s += 30

        # Most loops someone uses the cooler; cold is the most popular.
        if random.getrandbits(2) != 0:
            temperature = random.choice(("cold", "cold", "hot", "ambient"))
            dispenser.dispense_water(temperature)

        # Someone refills a tank every so often.
        if beat % 15 == 14:
            dispenser.tank_refilled(random.choice(("cold", "hot")))

        # Toggle the child lock occasionally (e.g. cleaning staff).
        if beat % 20 == 19:
            dispenser.child_lock_toggled(not dispenser.child_lock)

        if dispenser.filter_life_percent <= FILTER_LOW_PERCENT:
            dispenser.filter_low()
            if dispenser.filter_life_percent == 0:
                dispenser.filter_life_percent = 100  # filter replaced

        if beat % 5 == 0:
            dispenser.temperature_reading()
        if beat % HEARTBEAT_EVERY == 0:
            dispenser.heartbeat()

        beat += 1
        time.sleep(1)


# On a board, main.py runs automatically at boot.
run()
