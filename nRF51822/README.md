nRF51822 Codebase
=================

This repository includes everything you need to build nRF51822 firmware using the GNU toolchain and, where possible, open source tools.

Nordic Tools/SDK and SoftDevice
===============================

Before you can build any code for the nRF51822, you will need to download Nordic's **S110 SoftDevice**. The SoftDevice contains Nordic's Bluetooth Low Energy stack, which sits below your custom user code.

For license reasons, we are not able to include the SoftDevice in the codebase, nor the JLinkARM.dll file from Segger that some of the Nordic tools rely on.  These will need to be downloaded from the respective companies after accepting the appropriate license terms and conditions.

Mandatory Tools
---------------

The following tools are required regardless of the toolchain you are using (GNU Tools or Keil uVision):

- [Segger J-Link Drivers (v4.80 for example)](http://www.segger.com/jlink-software.html)
- **S110 SoftDevice** (v5.2.1) (available after registering your nRF51822 board on [Nordic's website](http://www.nordicsemi.com/))

Downloaded and install the J-Link drivers if you don't already have a recent copy installed, and then download the S110 SoftDevice, which will be placed in an appropriate folder in this codebase (see below for the exact location).
  
Optional Tools
--------------

- The **nRF51822 SDK** (available after registering your nRF51822 board on [Nordic's website](http://www.nordicsemi.com/))

The nRF51822 SDK is only mandatory if you want to use Keil uVision, since it includes support files for Keil (v4.xx only, v5.xx is not yet supported).

If you only plan on using GNU tool and the GNU/GCC toolchain this is an optional download since we have already included a copy of the SDK in the codebase.  The SDK also includes the **nrfjprog.exe** utility which allows you to program the flash from the command-line, but this utility has also been included in the codebase at '/tools/Windows/nordic'.

If you do install the SDK, **be sure to use the Windows installer for this rather than the .zip file**, since there are several steps in the installation process that aren't handled by the .zip file!

Installation Order
------------------

You **MUST** install the J-Link drivers **BEFORE** installing the nRF51822 SDK or you will have problems with path locations looking for the JLinkARM.dll when using the nrfjprog.exe utility!  This utility uses the J-Link .dll, and is required to program the flash from the command-line.  If you run into this problem, try uninstalling the SDK, reinstall the J-Link drivers and then re-install the Nordic SDK.

Adding the SoftDevice to the Codebase
-------------------------------------

The SoftDevice must be unzipped an placed in the /lib/softdevice/* folder, and shown below. This allows multiple versions of the SoftDevice to co-exist in the codebase, and each Makefile can point to the appropriate header files for the softdevice used by that project:

```
  [projfolder]/lib/softdevice/s110_nrf51822_5.2.1/s110_nrf51822_5.2.1_softdevice.hex
```

Make sure that you also include the matching SoftDevice header files, which will normally go in the folder below:

```
  [projfolder]/lib/softdevice/s110_nrf51822_5.2.1/s110_nrf51822_5.2.1/s110_nrf51822_5.2.1_API/include
```

GNU Build Tools
===============

If you don't already have a valid GNU toolchain and/or supporting build tools installed (make, etc.), we recommend using the following tools with the nRF51822 on Windows:

- [GNU Tools for ARM Embedded Processors 4.8-2013-q4](https://launchpad.net/gcc-arm-embedded/4.8/4.8-2013-q4-major)
- [Coreutils for Windows 5.3.0](http://gnuwin32.sourceforge.net/packages/coreutils.htm)
- [Make for Windows 3.81](http://gnuwin32.sourceforge.net/packages/make.htm)

Adding GNU Build Tools to %PATH%
--------------------------------

To access the above tools once they are installed, you need to add the installation folder to the %PATH% environment variable.  

Depending on your system and installation folder, the location that you need to add to %PATH% should resemble the following:

- C:\Program Files (x86)\GnuWin32\bin

You can type 'make --version' to see if the build tools were properly installed and added to your path variable.

Update Makefile.windows to point to your Toolchain
--------------------------------------------------

The last thing you need to do is update Makefile.windows, which is a global config file located at lib/sdk/[SDKVersion]/Nordic/nrf51822/Source/templates/gcc.

This file needs to be updated with the folder where you installed the GNU ARM toolchain, as well as the toolchain version number.  This allows the makefile to point to the right toolchain if multiple versions are present on your system.

If you used the toolchain mentionned above, your config settings should resemble the following values:

```
  GNU_INSTALL_ROOT := C:/Program Files (x86)/GNU Tools ARM Embedded/4.8 2013q4/
  GNU_VERSION := 4.8.3
  GNU_PREFIX := arm-none-eabi
```
