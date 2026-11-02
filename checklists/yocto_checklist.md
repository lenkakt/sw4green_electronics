# Yocto Project/OpenEmbedded checklist

Derived from the [main checklist](main_checklist.md). Reference system: the [meta-green layer](../demos/yocto/meta-green/README.md), which is a normal embedded Linux with the `green` distribution on `qemuarm`.

Run the build commands in an initialised build directory, and the target commands inside `runqemu`.

**Limit of QEMU:** `qemuarm` shows configuration and behaviour, not real power consumption or real watchdog hardware.

**Status** says what the layer does today: **Ready** means it is implemented in the layer. **Your project** and **Planned** mean it is not.

| No. | What to look for | How to verify | Status |
|---|---|---|---|
| **1** | **Reduce** | | |
| 1.1 | Kernel idle power features are enabled, in a separate fragment. | On target: `zcat /proc/config.gz \| grep -E "NO_HZ_IDLE\|CPU_IDLE"`. | Ready |
| 1.2 | Services do not poll. | On target: `top` shows the system almost fully idle after boot. | Ready |
| 1.3 | Only needed services and distribution features. | On target: `systemctl list-unit-files --state=enabled`. Check `DISTRO_FEATURES` with `bitbake-getvar DISTRO_FEATURES`. | Ready |
| **2** | **Reliability** | | |
| 2.1 | Each layer has a single purpose. | `bitbake-layers show-layers` and `yocto-check-layer meta-green`. | Ready |
| 2.2 | Library ABI changes are visible. | `buildhistory` is on. Compare two builds with `buildhistory-diff`. | Ready |
| 2.3 | Logging with limited retention, and a watchdog. | On target: `journalctl --disk-usage` and `systemctl show -p RuntimeWatchdogUSec`. | Ready (software watchdog on QEMU) |
| 2.4 | All inputs are pinned and stored. | `grep -rn AUTOREV meta-green` returns nothing. `demos/yocto/green-qemuarm.conf.json` pins BitBake, openembedded-core and meta-yocto to commits. Rebuild with `BB_NO_NETWORK = "1"`. Compare builds with `oe-selftest -r reproducible`. | Partly ready: also pin your other layers |
| 2.5 | Works across machines from different vendors. | `bitbake-config-build enable-fragment machine/qemuarm64`, then `bitbake -g green-image`. Repeat for `machine/qemuriscv64`. Use mainline layers and a vendor BSP only where needed. | Ready |
| 2.6 | Signed updates, rollback, SBOM, and vulnerability checks. | SPDX files appear in `tmp/deploy/images/`. Add `INHERIT += "cve-check"` to get a CVE report. Signing and A/B updates: see the layer README. | SBOM ready; signing and A/B planned |
| **3** | **Reuse** | | |
| 3.1 | The layer works in another build. | Add it to another setup with `bitbake-layers add-layer` and build. | Ready |
| 3.2 | The layer documents its interface. | The README lists dependencies, the distribution, the image and how to use them. | Ready |
| **4** | **Repair** | | |
| 4.1 | Machine details stay out of the layer. | `grep -rn qemuarm meta-green/conf meta-green/recipes-*` returns nothing. | Ready |
| 4.2 | Changing the board does not change the image. | Enable another `machine/` fragment and build again. Only the BSP changes. | Ready |
| 4.3 | Distribution, kernel options and machine are separate. | Distribution settings are in `conf/distro/`, kernel options in fragments, and the machine in the BSP layer. | Ready |
| 4.4 | A rebuild for new hardware is documented. | Enable the new `machine/` fragment. For real hardware, add a BSP layer and each board's documentation. | Your project |
| 4.5 | Updates in the field. | Add `meta-rauc` or `meta-mender`, install an update and check `/etc/os-release`. | Planned |
