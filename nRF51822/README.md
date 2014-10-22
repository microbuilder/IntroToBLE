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

- [Segger J-Link Drivers](http://www.segger.com/jlink-software.html)
- The **nRF51822 SDK** (available after registering your nRF51822 board on [Nordic's website](http://www.nordicsemi.com/))
- **S110 SoftDevice** (v6.0.0) (available after registering your nRF51822 board on [Nordic's website](http://www.nordicsemi.com/))

**Be sure to use the Windows installer for the SDK rather than the .zip file**, since there are several steps in the installation process that aren't handled by the .zip file!

Installation Order
------------------

You **MUST** install the J-Link drivers **BEFORE** installing the nRF51822 SDK or you will have problems with path locations looking for the JLinkARM.dll when using the nrfjprog.exe utility!  This utility uses the J-Link .dll, and is required to program the flash from the command-line.  If you run into this problem, try uninstalling the SDK, reinstall the J-Link drivers and then re-install the Nordic SDK.

Optional Tools
--------------

This codebase includes the **nrfjprog.exe** utility from Nordic (/tools/Windows/nordic) which allows you to program the flash from the command-line.

Adding the SoftDevice and SDK to the Codebase
---------------------------------------------

The SoftDevice must be unzipped and placed in the /lib/softdevice/* folder, and shown below. This allows multiple versions of the SoftDevice to co-exist in the codebase, and each Makefile can point to the appropriate header files for the softdevice used by that project:

```
  [projfolder]/lib/softdevice/s110_nrf51822_6.0.0/s110_nrf51822_6.0.0_softdevice.hex
```

Make sure that you also include the matching SoftDevice header files, which will normally go in the folder below:

```
  [projfolder]/lib/softdevice/s110_nrf51822_6.0.0/s110_nrf51822_6.0.0_API/include
```

Similarly, the SDK files must be placed in the following folder, which allows you to maintain several versions of the SDK across different projects if necessary:

```
  [projfolder]/lib/sdk/nRF51_SDK_v5.2.0.39364
```

GNU Build Tools
===============

If you don't already have a valid GNU toolchain and/or supporting build tools installed (make, etc.), we recommend using the following tools with the nRF51822 on Windows:

- [GNU Tools for ARM Embedded Processors 4.8-2013-q4](https://launchpad.net/gcc-arm-embedded/4.8/4.8-2013-q4-major)
- [Coreutils for Windows 5.3.0*](http://gnuwin32.sourceforge.net/packages/coreutils.htm)
- [Make for Windows 3.81*](http://gnuwin32.sourceforge.net/packages/make.htm)

*IMPORTANT NOTE*: When installing 'GNU Tools for ARM Embedded Processors', be sure to install the toolchain in a folder *that doesn't contain any spaces or special characters*.  This is important to avoid problems with the makefiles and compilation process. For example, you might want to consider installing everything in 'C:\ARM\4.8_2013q4'.

*You can also try a package like [mingw](http://www.mingw.org/) to provide the required build tools on Windows.

Adding GNU Build Tools to %PATH%
--------------------------------

To access the above tools once they are installed, you need to add the installation folder to the %PATH% environment variable.

Depending on your system and installation folder, the location that you need to add to %PATH% should resemble the following:

- C:\Program Files (x86)\GnuWin32\bin

You can type 'make --version' to see if the build tools were properly installed and added to your path variable.

Update Makefile.windows to point to your Toolchain
--------------------------------------------------

The last thing you need to do is update Makefile.common, which is located at projects/Makefile.common.

This file needs to be updated with the folder where you installed the GNU ARM toolchain, as well as the toolchain version number.  This allows the makefile to point to the right toolchain if multiple versions are present on your system.

If you used the toolchain mentionned above, your config settings should resemble the following values:

```
  GNU_INSTALL_ROOT := C:/ARM/4.8_2013q4
  GNU_VERSION := 4.8.3
  GNU_PREFIX := arm-none-eabi
```
