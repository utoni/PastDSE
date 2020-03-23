@echo off
set SCNAME=PastDSE
set DRIVER="%~dp0\x64\Debug\PastDSEDriver.sys"
if not exist %DRIVER% set DRIVER="%~dp0\bin\x64\Debug\PastDSEDriver.sys"

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
echo Driver......: %DRIVER%
echo ***************************

sc create %SCNAME% binPath= %DRIVER% type= kernel
echo ***************************
sc start %SCNAME%
echo ***************************
sc query %SCNAME%

REM pause
timeout /t 3