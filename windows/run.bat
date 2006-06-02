@ECHO OFF
cls

ECHO ---------------------- The LOST Windows make file ----------------------
ECHO  Run the Lowlevel Operating System:
ECHO ------------------------------------------------------------------------
ECHO   - Copy the (new) boot Image
ECHO   - Run the boot Image with bochs
ECHO ------------------------------------------------------------------------
PAUSE

IF EXIST "..\debug\ide\ata0-master.img" del "debug\ide\ata0-master.img"
copy "..\image\image.img" "debug\ide\ata0-master.img"

cd "..\debug"
call "run.bat"
cd..

EXIT