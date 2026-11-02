# Zephyr checklist

Derived from the [main checklist](main_checklist.md). Reference application: the [greenhouse controller](../demos/zephyr/greenhouse/README.md).

**Status:** the greenhouse application is only sketched so far, so these checks are defined but have not been run yet.

**QEMU:** with the QEMU option, you can run 1.2, 2.1, 2.2, 2.4, 2.5, 3.1, 3.2 and 4.1 to 4.3 without hardware. The rest need the real board.

| No. | What to look for | How to verify on the reference application |
|---|---|---|
| **1** | **Reduce** | |
| 1.1 | Battery life: threads sleep between work, and the system enters low-power states. | `CONFIG_PM` and `CONFIG_PM_DEVICE` are in `build/zephyr/.config`. `CONFIG_PM_STATS` shows light sleep in use. Measure the current on the board. |
| 1.2 | Every thread loop blocks on `zbus_sub_wait()` or `k_sleep()`. | Review each `while (1)` in `src/`: `grep -n "while (1)\|while (true)" src/*.c`. No loop may poll without blocking. |
| 1.3 | Battery life: sensors sleep between samples, and Wi-Fi uses power save. | `CONFIG_BME280_MODE_FORCED=y` is set. The `soil-power` GPIO is on only while sampling. Check Wi-Fi power save with `wifi ps` in the shell. |
| **2** | **Reliability** | |
| 2.1 | One module per file, connected only by zbus. | `grep` for calls between modules: a module includes another module's header only to reach its zbus channel. |
| 2.2 | Module interfaces and message structures are versioned. | Structures on zbus channels carry a version field, and releases are tagged with semantic versions. |
| 2.3 | Maintenance: logging per module, kept across resets, plus a health table and a watchdog with thread heartbeats. | Raise a module's level with `log enable dbg <module>` in the shell. Force a reset, then read the reason from the stored log. Run `app_status` and `app_watchdog status`. Stop one thread: the watchdog must reset the device. |
| 2.4 | The Zephyr version and all modules are pinned. | `west.yml` uses fixed revisions, and `west manifest --freeze` matches it. A clean rebuild from a new checkout works by following the README. |
| 2.5 | Only Zephyr APIs and open protocols. | No vendor HAL calls in `src/`. The backend uses plain HTTP with InfluxDB line protocol over POSIX sockets. Build for a second board, for example the QEMU target. |
| 2.6 | Signed images, rollback and a version number. | Build with MCUboot and sign with imgtool. An unsigned image must be rejected. An image that does not confirm itself must roll back. |
| **3** | **Reuse** | |
| 3.1 | Modules work outside this application. | Use `app_watchdog`, `app_status` and `app_wifi` unchanged in another project. |
| 3.2 | Each module's interface is documented. | Each `app_*.h` describes its functions, and each zbus channel lists its publisher and subscribers. |
| **4** | **Repair** | |
| 4.1 | Only Zephyr's generic sensor and GPIO APIs in application code. | `grep -n "bme280\|bh1750" src/*.c` returns nothing. Soil moisture uses the generic ADC API. |
| 4.2 | A different sensor needs no change to the code. | Replace the BME280 with an SHT4x in the overlay and rebuild. `git diff --stat` shows only `boards/`. |
| 4.3 | Hardware is described only in Devicetree. | The application uses `DT_ALIAS(...)` for `env-sensor`, `light-sensor`, `soil-sensor`, `water-valve` and `vent-fan`. |
| 4.4 | Settings are kept across a repair, and the build is one command. | Device ID, thresholds, soil calibration, Wi-Fi and token are in Settings/NVS. `west build` with the new overlay is enough. |
| 4.5 | Updates over the network. | Upload a new image with mcumgr, and check the new version with `app_device` in the shell. |
