@echo off

set SIGNTOOL="C:\Program Files (x86)\Windows Kits\10\bin\10.0.17763.0\x64\signtool.exe"
set DRIVER_DEBUG="%~dp0x64\Debug\PastDSEDriver.sys"
set DRIVER_RELEASE="%~dp0x64\Release\PastDSEDriver.sys"
set CA="%~dp0\VeriSign Class 3 Public Primary Certification Authority - G5.cer"
set CERT="%~dp0\cert.pfx"

date 01-01-14

echo ***************************
echo workdir.: %cd%
echo signtool: %SIGNTOOL%
echo driver..: %DRIVER%
echo ca......: %CA%
echo cert....: %CERT%
echo ---------------------------
%SIGNTOOL% sign /a /ac %CA% /f %CERT% %DRIVER_DEBUG%
%SIGNTOOL% sign /a /ac %CA% /f %CERT% %DRIVER_RELEASE%
echo ***************************
%SIGNTOOL% verify /kp /v %DRIVER_DEBUG%
%SIGNTOOL% verify /kp /v %DRIVER_RELEASE%
echo ***************************

net stop w32time
net start w32time
w32tm /resync /nowait

pause