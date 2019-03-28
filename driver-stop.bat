@echo off
set SCNAME=PastDSE

echo ***************************
echo Service Name: %SCNAME%
echo ***************************

sc stop %SCNAME%
echo ***************************
sc delete %SCNAME%

REM fsutil usn deleteJournal /D C:
REM pause
timeout /t 3