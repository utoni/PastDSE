@echo off
set SCNAME=PastDSE

net session >nul 2>&1
if %ERRORLEVEL% EQU 0 (
	echo You are running this script as admin. Fine!
) else (
	echo ERROR: This script requires admin privileges!
	pause
	exit /b 1
)

echo ***************************
echo Service Name: %SCNAME%
echo ***************************

sc stop %SCNAME%
echo ***************************
sc delete %SCNAME%

REM fsutil usn deleteJournal /D C:
REM pause
timeout /t 3