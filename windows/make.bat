@ECHO OFF
cls

ECHO ---------------------- The LOST Windows make file ----------------------
ECHO  Compile the Lowlevel Operating System:
ECHO ------------------------------------------------------------------------
ECHO  Compile all Kernel modules
ECHO    - Core ???
ECHO ------------------------------------------------------------------------
ECHO               (C) 2006 ??              
ECHO ------------------------------------------------------------------------

PAUSE
cd..

cd "load_process\bootloader"
call "make.bat"
cd..
PAUSE

cls

ECHO ---------------------- The LOST Windows make file ----------------------
ECHO  Compilation of the Lowlevel Operating System is completed:
ECHO ------------------------------------------------------------------------
ECHO  Compile all Kernel modules
ECHO    - Core ???
ECHO ------------------------------------------------------------------------
ECHO               (C) 2006 ??              
ECHO ------------------------------------------------------------------------

EXIT