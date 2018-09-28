cl /I .. ../main.c rs232-win.c /link /out:flash.exe

del *.obj

move flash.exe ../../bin/windows32/flash.exe


