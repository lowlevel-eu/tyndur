..\..\win-tools\bin\nasm.exe "bootloader.asm" -o "bootloader.sys" -l "bootloader.lst" -f bin -O99"
move "*.sys" "..\..\build\bootloader"
move "*.lst" "..\..\build\bootloader"
cmd