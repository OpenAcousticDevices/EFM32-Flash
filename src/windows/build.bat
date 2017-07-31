cl /I .. ../main.c rs232-win.c /link /out:flash.exe

del *.obj

move flash.exe ../../bin/windows/flash.exe


