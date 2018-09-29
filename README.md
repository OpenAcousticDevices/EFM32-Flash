# Flash #

This is a command line tool for accessing flashing EFM32 devices using the factory supplied USB bootloader. 

## How to use it ##

To list the available ports use:

```
> ./flash 
/dev/tty.usbmodem1421
```

To access the serial number of the connected device:

```
> ./flash -i /dev/tty.usbmodem1421
Serial Number: 24410E05588EF20C
```

To check the CRC of the current flash. 

```
> ./flash -c /dev/tty.usbmodem1421
Flash CRC: 40AA
```

To upload a binary file to the device. 

```
> ./flash -u /dev/tty.usbmodem1421 AudioMoth1.0.0.bin
Programmed: 41412 bytes
Flash CRC: 4393
```

This is a non-destructive write which leaves the bootloader in place. The '-d' option will overwrite the bootloader. You should only use this option if you know what you are doing as you will need to use a JTAG programmer to recover the device if anything goes wrong. 

Note that the default CRC is that of the application only. When a destructive write is performed the reported CRC is that of the entire flash.

From Node.js or Python the command line tool can be called as a child process.

### macOS ###

macOS should already include the necessary USB CDC serial port driver.

### Windows ###

Windows 10 will automatically install the necessary USB CDC serial port driver. On Windows 7 you will need to manually install the driver using EFM32-Cdc.inf file included in this repository. 

### Linux ###

Most Linux distributions should already include the necessary USB CDC serial port driver. You might also need to change the permissions of the port before you can access it.

```
> sudo chmod 666 /dev/ttyACM0
```

## Building ##

On macOS use the Xcode project to build the binary. After archiving, the resulting executable must be manually moved to the correct folder.

On Linux use gcc to build the binary by running build.sh. This will copy the executable to the correct folder. Or compile manually. 

```
> gcc -Wall -std=c99 -I../.. -o flash ../../main.c ../rs232.c
```

On Windows use the Microsoft Visual C++ Build Tools on the command line by running build.bat or build32.bat. This will copy the executable to the correct folder. 

Or compile manually. 

```
> cl /I .. ../main.c rs232-win.c /link /out:flash.exe
```

Note that to build the correct version you should run the command in the correct environment. Use the 'x64 Native Tools Command Prompt' to build the 64-bit binary on a 64-bit machine, and the 'x64_x86 Cross Tools Command Prompt' to build the 32-bit binary on a 64-bit machine.

## More Information ##

The Silicon Labs USB bootloader is described in an Application Note [here](https://www.silabs.com/documents/public/application-notes/an0042-efm32-usb-uart-bootloader.pdf).
