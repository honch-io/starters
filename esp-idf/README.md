# ESP-IDF starter — connected espresso machine

A self-contained simulation of the firmware for a Wi-Fi connected coffee machine
running on an ESP32. It models the consumables and state a real machine tracks
(water tank, bean hopper, boiler temperature, descale counter) and runs a
"barista" loop on a FreeRTOS task that pulls shots all day: starting and
completing brews, frothing milk for milk drinks, warning when the tank or beans
run low, asking to be descaled, running cleaning cycles, and occasionally hitting
a hardware fault.

It's an ordinary ESP-IDF app with **no Honch SDK** in it. It's the kind of real
firmware the [Honch setup wizard](https://github.com/honch-io/wizard)
(`npx @honch/start`) integrates analytics into — or that you can read, build, and
flash directly.

## Build & run

```sh
idf.py set-target esp32 && idf.py build
idf.py -p <PORT> flash monitor
```

Watch the serial monitor: the machine logs each brew, cleaning cycle, and
heartbeat as it runs.

## What it does

`main/app_main.c` holds the machine state (`machine_t`) and one function per
device action:

| Action | Function |
| --- | --- |
| Power on | `machine_power_on()` |
| Start a brew | `brew_started()` |
| Finish a brew | `brew_completed()` |
| Froth milk (milk drinks) | `milk_frothed()` |
| Low water / beans | `water_tank_low()` / `beans_low()` |
| Descale reminder | `descale_due()` |
| Cleaning / descale cycle | `cleaning_cycle_run()` |
| Hardware fault | `brew_error()` |
| Health heartbeat | `heartbeat()` |

`barista_task()` ties them together, pulling one drink per loop and topping up
consumables so the demo runs indefinitely.
