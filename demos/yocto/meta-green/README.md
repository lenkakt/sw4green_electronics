# meta-green

A Yocto Project/OpenEmbedded distribution layer applying the [green checklist](../../../checklists/yocto_checklist.md). It holds only distribution policy, and has no machines and no images.

| Item | Name | Purpose |
|---|---|---|
| Distribution | `green` | Poky without unneeded features, systemd, build history and repeatable sources |
| Recipe | `green-system-config` | Limited log size and a watchdog |
| Kernel fragments | `green-power.cfg`, `green-diagnostics.cfg` | Idle power and diagnostics options |

**Depends on:** `meta` (openembedded-core), `meta-poky` (meta-yocto). **Compatible with:** `wrynose`.

## The reference system has three kinds of layer

| Kind | Layer | Holds |
|---|---|---|
| Distribution | `meta-green` | Policy |
| Product | [meta-green-demo](../meta-green-demo/README.md) | Image and package set |
| BSP | `meta` provides `qemuarm` | Machine |

Each can be replaced without touching the others. Your own product needs `green-system-config` in its image, as `packagegroup-green` does here.

## Power

`green-power.cfg` follows [Tuning Embedded Linux for Low Power](https://fosdem.org/2026/schedule/event/CPCNNE-tuning_embedded_linux_for_low_power/) (Kévin L'hôpital, FOSDEM 2026). The talk's measurements are from an i.MX8, so yours will differ.

| Technique | Saving in the talk | Here |
|---|---|---|
| Suspend to idle | 12.5% | Enabled (`CONFIG_SUSPEND`) |
| Suspend to RAM | 50% | Enabled, if your platform supports it |
| CPU frequency scaling | 23% at full load, about none at low load | Governors enabled, default `schedutil` |
| Connectivity off | 52% | `wifi` and `bluetooth` removed from the distribution |
| Suspend to disk | 90% | Not enabled: it needs swap and a resume path in the bootloader |
| Power off | 90% | Your product's decision |
| USB autosuspend | 56% on a USB modem | Not enabled: some devices do not resume, so test yours first |

Enabling a feature does not use it. **Your product decides when to suspend and what wakes it.** A real board also needs its own CPU frequency and idle drivers from its BSP layer.

## Use it

See the [parent README](../README.md).

## Planned

- **A/B updates:** add `meta-rauc` or `meta-mender`, with a rollback-capable partition layout.
- **Signing:** package and boot artefact signing needs your own keys, so it is not enabled here.
