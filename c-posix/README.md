# C/POSIX starter — vending machine controller

A small, self-contained simulation of the long-lived controller daemon that
runs on the embedded-Linux brain of a connected vending machine. It models
product slots, a refrigerated cabinet, and a cash box, then drives a stream of
realistic activity — sales, failed vends, technician service visits, restocks,
cash collection, temperature alarms, and a periodic heartbeat.

It's a plain C program with **no Honch SDK** in it. It's the kind of real device
codebase the [Honch setup wizard](https://github.com/honch-io/wizard)
(`npx @honch/start`) integrates analytics into — or that you can read, run, and
build on directly.

## Run it

```sh
cmake -S . -B build && cmake --build build && ./build/app
```

You'll see a live log of the machine's day. Press `Ctrl-C` to stop (the
controller shuts down gracefully).

## What it does

`main.c` holds the controller state (`controller_t`) and one function per device
action:

| Action | Function |
| --- | --- |
| Boot | `controller_start()` |
| Customer purchase | `vend_product()` |
| Failed purchase | (logged from `vend_product()`) |
| Restock a slot | `restocked()` |
| Technician opens machine | `service_access()` |
| Empty the cash box | `cash_collected()` |
| Cabinet temperature check / alarm | `check_temperature()` |
| Health heartbeat | `heartbeat()` |
| Graceful shutdown | `controller_stop()` |

The main loop ties them together, simulating one "minute" of machine time per
tick.
