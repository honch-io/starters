 # 🛠  `starters`, Honch project examples

Runnable starter projects the [Honch setup wizard](https://github.com/honch-io/wizard)
(`npx @honch/start`) can scaffold when you run it in an empty directory ("Try Honch").

Each folder is a small **device simulation** for one SDK target, written as
ordinary, idiomatic code with **no Honch SDK in it** — the wizard scaffolds the
project and then its agent wires Honch in. You can also just clone a folder and
run it directly.

| Folder | Target | Simulated device |
| --- | --- | --- |
| `c-posix/` | C / POSIX | Vending machine controller (long-lived daemon) |
| `micropython/` | MicroPython | Cold/hot water dispenser |
| `esp-idf/` | ESP-IDF (ESP32) | Connected espresso machine |

Each simulation models real device state and drives a stream of realistic
activity — purchases, brews, faults, refills, heartbeats — so there are
meaningful, natural points to capture analytics from. See each folder's
`README.md` for what it does and how to run it.

## Build status

| Folder | Verified |
| --- | --- |
| `c-posix/` | Builds and runs with CMake + a C11 compiler |
| `esp-idf/` | Builds with ESP-IDF (`idf.py build`) |
| `micropython/` | Syntax-checked; flash to a board to run (uses `machine`/`time`) |
