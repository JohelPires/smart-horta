# smart-horta

ESP32 smart-gardening controller: reads soil/water/climate sensors, drives
irrigation valves, and publishes telemetry over MQTT to a host
(computer/Raspberry Pi). Monorepo, currently being scaffolded.

## Repo layout (target)

| Path        | Contents                                                              |
| ----------- | --------------------------------------------------------------------- |
| `firmware/` | ESP-IDF project: `main/`, `components/` (hal, sensors, …), `wokwi.toml`, `diagram.json` |
| `host/`     | Python MQTT subscriber, storage, dashboard/CLI                        |
| `docs/`     | Wiring, calibration, MQTT topic reference                             |

## ESP-IDF activation — why it's manual

ESP-IDF is installed at `~/esp-idf` with its toolchain in `~/.espressif`, but it
is **not** auto-sourced from `~/.bashrc`. Every new shell stays clean unless you
activate it manually.

To work on the firmware, run **once per new shell**:

```bash
idfenv          # alias defined in ~/.bashrc; equivalent to:
                # source ~/esp-idf/export.sh
```

Then, from `firmware/`:

```bash
idf.py set-target esp32
idf.py build
idf.py -p /dev/ttyUSB0 flash monitor
wokwi-cli . --timeout 10000   # Wokwi simulation (not `idf.py wokwi`)
```

## Why not auto-source ESP-IDF?

Sourcing `~/esp-idf/export.sh` in every shell caused real problems for
non-ESP-IDF work (backend development in other projects):

- **Bare `python3`/`pip` get shadowed** by the IDF Python virtualenv
  (`~/.espressif/python_env/idf5.5_py3.12_env`) in *every* project — a stray
  `pip install` outside a project venv polluted the IDF environment instead of
  the system.
- **`PATH` bloat**: ~15 ESP tool entries (xtensa-esp-elf, openocd, esp-gdb, …)
  are prepended in every shell, where any name collision silently wins.
- **Startup cost**: the export.sh banner adds ~1–2 s to every new terminal.

Manual activation via `idfenv` keeps all other projects unaffected: clean `PATH`,
your normal system Python/pip, and no banner unless you ask for it. Tools and
agents working in this repo should run `idfenv` (or `source ~/esp-idf/export.sh`)
in the same shell before any `idf.py`/`wokwi` command.
