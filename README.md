# Flash #

This is a command line tool for accessing flashing EFM32 devices using the USB bootloader. 

### How to use it ###

To list the available ports use:

```
#!basic
> ./flash 
/dev/tty.usbmodem1421
```

The response is 'NULL' if no ports are found, and 'ERROR: message' if something goes wrong.

To access the serial number of the connected device:

```
#!basic
> ./flash -i /dev/tty.usbmodem1421
Serial Number: 24410E05588EF20C
```
To check the CRC of the current flash. 

```
#!basic
> ./flash -c /dev/tty.usbmodem1421
Flash CRC: 40AA
```

To write a binary file to the device. 

```
#!basic
> ./flash /dev/tty.usbmodem1421 AudioMoth.bin
Programmed: 42304 bytes
Flash CRC: 40AA
```

The default is a non-destructive write. The '-d' option will overwrite the bootloader. Note that the default CRC is that of the application only. When a destructive write is performed the reported CRC is that of the entire flash.

From Node.js or Python the command line tool can be called as a child process.

### Linux ###

The executable will work as is on macOS and Windows. However, Linux prevents USB HID devices from being writable by default. This can be fixed by adding an additional rule in /lib/udev/rules.d/. For AudioMoth, the following additional rule file, called 99-audiomoth.rules, is used. The content of which is:

```
#!python
SUBSYSTEM=="usb", ATTRS{idVendor}=="10C4", ATTRS{idProduct}=="0003", MODE="0666"
```

You might also need to change the permissions of the port.

```
#!basic
> sudo chmod 666 /dev/ttyACM0
```

### Building ###

On macOS use the Xcode project to build the binary. After archiving, the resulting executable must be manually moved to the correct folder.

On Linux use gcc to build the binary by running build.sh. This will copy the executable to the correct folder. Or compile manually. 
```
#!basic
> gcc -Wall -std=c99 -I../.. -o flash ../../main.c ../rs232.c
```

On Windows use the Microsoft Visual C++ Build Tools on the command line by running build.bat. This will copy the executable to the correct folder. Or compile manually. 
```
#!basic
> cl /I .. ../main.c rs232-win.c /link /out:flash.exe
```