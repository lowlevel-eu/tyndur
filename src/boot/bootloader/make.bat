@ECHO OFF
cls

ECHO ---------------------- The LOST Windows make file ----------------------
ECHO  Compile the "Bootloader":
ECHO ------------------------------------------------------------------------

..\..\win-tools\bin\nasm.exe "bootloader.asm" -o "bootloader.sys" -l "bootloader.lst" -f bin -O99"
@ECHO ON
@ECHO OFF

ECHO ------------------------------------------------------------------------


move "*.sys" "..\..\build\bootloader"
move "*.lst" "..\..\build\bootloader"
cmd