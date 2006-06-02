@ECHO OFF
cls

ECHO ---------------------- The LOST Windows make file ----------------------
ECHO  Create the Image file:
ECHO ------------------------------------------------------------------------
ECHO    - Copy all System files
ECHO    - assemble the Image
ECHO    - Delete all System files
ECHO ------------------------------------------------------------------------

PAUSE

cd "..\image"
call "make.bat"
cd..

EXIT