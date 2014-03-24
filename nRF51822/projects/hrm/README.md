Project Name
============

About this project

Building this Project
=====================

To build a clean copy of your project in release mode, enter the same folder as your Makefile on the command-line and enter the following command:

- make clean release

Other relevant make targets are:

- make
- make debug
- make flash-softdevice (flashes Softdevice via JLink)
- make flash (flashes output hex file via JLink)
- make erase-all (erases flash)
- make startdebug - (starts GDBServer & GDB via JLink)
- make stopdebug - (stops GDBServer)
