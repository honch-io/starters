# Honch starters

Bare, minimal starter projects the [Honch setup wizard](https://github.com/honch-io/wizard)
(`npx @honch/start`) can scaffold when you run it in an empty directory ("Try Honch").

Each folder is a plain, runnable skeleton for one SDK target **with no Honch SDK
in it** — the wizard scaffolds the skeleton and then its agent wires Honch in.
You can also just clone a folder and start from it directly.

| Folder | Target |
| --- | --- |
| `c-posix/` | C / POSIX |
| `micropython/` | MicroPython |
| `esp-idf/` | ESP-IDF (ESP32) |

> ⚠️ These skeletons are intentionally minimal and have **not been build-verified
> against each toolchain yet** — verify before relying on them in production.
