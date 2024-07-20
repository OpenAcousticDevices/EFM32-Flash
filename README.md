# Flash #

This is a command line tool for accessing flashing EFM32 devices using the factory supplied USB bootloader. 

## How to use it ##

To list the available ports use:

```
#!basic
> ./flash 
/dev/tty.usbmodem1421
```

To access the serial number of the connected device.

```
#!basic
> ./flash -i /dev/tty.usbmodem1421
Serial Number: 24410E05588EF20C
```

To check the CRC of the contents of the flash of the connected device.

```
#!basic
> ./flash -c /dev/tty.usbmodem1421
Flash CRC: 40AA
```

To restart the connected device.

```
#!basic
> ./flash -r /dev/tty.usbmodem1421
```

To upload a binary file to the flash of the connected device.

```
#!basic
> ./flash -u /dev/tty.usbmodem1421 AudioMoth1.0.0.bin
Programmed: 41412 bytes
Flash CRC: 4393
```

This is a non-destructive write which leaves the bootloader in place. The '-d' option will overwrite the bootloader. You should only use this option if you know what you are doing as you will need to use a JTAG programmer to recover the device if anything goes wrong. 

Note that the default CRC is that of the application only. When a destructive write is performed the reported CRC is that of the entire flash.

From Node.js or Python the command line tool can be called as a child process.

### macOS ###

macOS already includes the necessary USB CDC serial port driver.

### Windows ###

On Windows 7 you will need to manually install the driver using EFM32-Cdc.inf file included in this repository. Later versions of Windows will automatically use an appropriate USB CDC serial port driver. 

### Linux ###

Linux distributions already include the necessary USB CDC serial port driver. On some distributions you may need to change the permissions of the port before you can access it.

By default, Linux prevents writing to certain types of USB devices such as the AudioMoth. To use this application you must first navigate to `/lib/udev/rules.d/` and create a new file (or edit the existing file) with the name `99-audiomoth.rules`:

```
> cd /lib/udev/rules.d/
> sudo gedit 99-audiomoth.rules
```

Then add the following text:

```
SUBSYSTEM=="tty", ATTRS{idVendor}=="10c4", ATTRS{idProduct}=="0003", MODE="0666"
```

On certain Linux distributions, you may also have to manually set the permissions for ports to allow the app to communicate with the AudioMoth. If you experience connection issues, try the following command:
​
```
> sudo usermod -a -G dialout $(whoami)
```

Finally, you may still have to manually set the permissions for ports to allow the app communicate with your AudioMoth. If you experience connection issues when trying to access a specific port (e.g. `/dev/ttyACM0`), try the following command:

```
> sudo chmod 666 /dev/ttyACM0
```

## Building from source ##

AudioMoth-Live can be built on macOS using the Xcode Command Line Tools.

```
> clang -I./src/ ./src/main.c ./src/macOS/rs232.c -o flash   
```

AudioMoth-Live can be built on Windows using the Microsoft Visual C++ Build Tools. Note that to build the correct version you should run the command in the correct environment. Use the 'x64 Native Tools Command Prompt' to build the 64-bit binary on a 64-bit machine, and the 'x64_x86 Cross Tools Command Prompt' to build the 32-bit binary on a 64-bit machine.

```
cl /I.\src\ .\src\main.c .\src\windows/rs232.c /link /out:flash.exe
```

AudioMoth-Live can be built on Linux and Raspberry Pi using the `gcc`.

```
> gcc -Wall -std=c99 -I./src/ ./src/main.c ./src/linux/rs232.c -o flash 
```

On macOS, Linux and Raspberry Pi you can copy the resulting executable to `/usr/local/bin/` so it is immediately accessible from the terminal. On Windows copy the executable to a permanent location and add this location to the `PATH` variable.

## Pre-built binaries ##

Pre-built binaries are also available in the repository.

## More Information ##

The Silicon Labs USB bootloader is described in an Application Note [here](https://www.silabs.com/documents/public/application-notes/an0042-efm32-usb-uart-bootloader.pdf).