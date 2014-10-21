Heart Rate Monitor
==================

This project instantiates a standard [Heart Rate Monitor service](https://developer.bluetooth.org/gatt/services/Pages/ServiceViewer.aspx?u=org.bluetooth.service.heart_rate.xml), which can be viewed using toos like nRF Toolbox from Nordic Semiconductors (available on iOS and Android in the respective app stores).

Target SDK/SD
=============

- **SDK**: v5.1.0
- **SoftDevice**: v6.0.0
- 
Building this Project
=====================

To build a clean copy of your project in release mode, enter the same folder as your Makefile on the command-line and enter the following command:

```
  make clean release
```

Other relevant make targets are:

- `make` is an alias for `make debug` below
- `make debug` builds a .hex file that includes debug data (larger, unoptimized binaries useful to debug via GDB Server)
- `make release` builds an optimized release version of the firmware
- `make clean` removes and previous build artifacts
- `make flash` flashes the nRF51 via JLink and nrfjprog.exe (currently Windows only)
