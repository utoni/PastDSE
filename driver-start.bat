@echo off
set SCNAME=PastDSE
set DRIVER="%~dp0\x64\Debug\PastDSEDriver.sys"
if not exist %DRIVER% set DRIVER="%~dp0\bin\x64\Debug\PastDSEDriver.sys"

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