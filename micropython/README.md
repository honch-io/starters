# MicroPython starter — cold/hot water dispenser

A self-contained simulation of the firmware for a connected water cooler running
MicroPython on a microcontroller (ESP32, RP2040, ...). It models a chilled tank
and a heated tank, a filter that wears out with use, and a child lock, then runs
a loop simulating a day at the office water cooler: people dispensing cold, hot,
or ambient water, tanks being refilled, the filter wearing low, the child lock
being toggled, periodic temperature readings, the odd hardware fault, and a
health heartbeat.

It's ordinary MicroPython firmware with **no Honch SDK** in it. It's the kind of
real device code the [Honch setup wizard](https://github.com/honch-io/wizard)
(`npx @honch/start`) integrates analytics into — or that you can read, flash, and
build on directly.

## Run it

Copy `main.py` to your board and let it run at boot, e.g. with
[`mpremote`](https://docs.micropython.org/en/latest/reference/mpremote.html):

```sh
mpremote connect auto fs cp main.py :main.py
mpremote connect auto run main.py
```

The dispenser logs each pour, refill, and heartbeat over the serial REPL.

## What it does

The `Dispenser` class holds the cooler state and has one method per device
action:

| Action | Method |
| --- | --- |
| Boot | `boot()` |
| Dispense cold/hot/ambient water | `dispense_water()` |
| Refill a tank | `tank_refilled()` |
| Filter wearing low | `filter_low()` |
| Toggle the child lock | `child_lock_toggled()` |
| Temperature reading | `temperature_reading()` |
| Hardware fault | `fault()` |
| Health heartbeat | `heartbeat()` |

`run()` ties them together, driving simulated activity once per second.

> Uses `machine` and `time` (MicroPython firmware modules), so it runs on a board
> rather than host CPython.
