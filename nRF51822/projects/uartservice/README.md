UART Service
============

This project shows how custom services and characteristics can be defined and manipulated on the nRF51822.

A sample 'UART' type service with two characteristics as follows:

- The first characteristic acts as a **TXD** line, with **indicate** enabled, which allows us to know definitively if the transmitted data was received or not.

- The second characteristic acts as the **RXD** line and has **write** enabled so that the connected GATT client device (the phone/tablet/etc.) can send data back to the GATT server (the nRF51822).

Target SDK/SD
=============

- **SDK**: v5.1.0
- **SoftDevice**: v6.0.0

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

