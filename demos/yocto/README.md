# Yocto Project/OpenEmbedded reference

A normal embedded Linux with a `green` distribution on `qemuarm`, built from two layers: [meta-green](meta-green/README.md) for the distribution and [meta-green-demo](meta-green-demo/README.md) for the image. It uses Yocto Project 6.0 "wrynose" (LTS).

Set it up with [bitbake-setup](https://docs.yoctoproject.org/bitbake/bitbake-user-manual/bitbake-user-manual-environment-setup.html). From a clone of this repository:

```
git clone -b 2.18 https://git.openembedded.org/bitbake
./bitbake/bin/bitbake-setup init --non-interactive demos/yocto/green-qemuarm.conf.json green
```

The command prints how to initialise the build environment. Then:

```
bitbake green-image
runqemu nographic slirp snapshot
```

`green-qemuarm.conf.json` pins BitBake, openembedded-core and meta-yocto to exact commits, which makes the setup repeatable. bitbake-setup records what it checked out in `layers/sources-fixed-revisions.json`.

## What has been tested

On `qemuarm`, Yocto Project 6.0 "wrynose":

| Test | Result |
|---|---|
| `bitbake-setup init` and a full build of `green-image` | Works |
| Boot, with the `green` distribution in `/etc/os-release` and `/etc/buildinfo` | Works |
| Kernel idle options and software watchdog in `/proc/config.gz` | Present |
| 5 minutes running: no failed units, no resets, idle load 0.04 | Works |
| Log size limit, network, SBOM (SPDX) files | Work |
| Parsing for `qemuarm64` | Works, not built |

A watchdog timeout of 30 s reset the system during a heavy `systemctl` call under emulation, so the timeout is 2 minutes. Set it to suit your hardware.

Not tested: `oe-selftest -r reproducible`, `yocto-check-layer` and building for other machines.

If you build on a host with `libgcc-14-dev` but without `libstdc++-14-dev`, BitBake's host check stops. Install `libstdc++-14-dev`.

## Switch the machine

```
bitbake-config-build enable-fragment machine/qemuarm64
bitbake green-image
```

The fragment replaces the current machine, and nothing in the layers changes. For real hardware, add its BSP layer to the setup first.

## Note

The configuration enables root login without a password, for the reference system only. Never use that on a real product.
