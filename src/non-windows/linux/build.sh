gcc -Wall -std=c99 -I../.. -o flash ../../main.c ../rs232.c

cp flash ../../../bin/linux/flash

rm -f flash
