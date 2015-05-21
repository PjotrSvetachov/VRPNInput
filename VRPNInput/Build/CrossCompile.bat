REM
REM Cross compile build script. We only use a small portion of VRPN so do not build it all.
REM

if not exist "linuxBuild" mkdir linuxBuild

REM Build vrpn library

SET vrpnFiles=vrpn_Analog vrpn_Analog_Output vrpn_Auxiliary_Logger vrpn_BaseClass vrpn_Button vrpn_Connection vrpn_Dial vrpn_FileConnection vrpn_FileController vrpn_ForceDevice vrpn_Forwarder vrpn_ForwarderController vrpn_FunctionGenerator vrpn_Imager vrpn_LamportClock vrpn_Mutex vrpn_Poser vrpn_RedundantTransmission vrpn_Serial vrpn_SerialPort vrpn_Shared vrpn_SharedObject vrpn_Sound vrpn_Text vrpn_Tracker

FOR %%a IN (%vrpnFiles%) do %LINUX_ROOT%/bin/clang++.exe -fPIC -Wno-switch -Iquat/ -Wno-unused-value -fvisibility=hidden -O3 -target x86_64-unknown-linux-gnu --sysroot=%LINUX_ROOT% -std=c++11 -o linuxBuild/%%a.o -c %%a.C
@echo off
SET NewCommand=

for %%a in (%vrpnFiles%) do call set "NewCommand=%%NewCommand%% linuxBuild/%%a.o"
@echo on
%LINUX_ROOT%/bin/x86_64-unknown-linux-gnu-ar.exe cr linuxBuild/vrpn.a %NewCommand%


REM Build quat library

SET quatFiles=matrix quat vector xyzquat
FOR %%a IN (%quatFiles%) do %LINUX_ROOT%/bin/clang++.exe -fPIC -Wno-switch -Iquat/ -Wno-unused-value -fvisibility=hidden -O3 -target x86_64-unknown-linux-gnu --sysroot=%LINUX_ROOT% -std=c++11 -o linuxBuild/%%a.o -c quat/%%a.C
@echo off
SET NewCommand=

for %%a in (%quatFiles%) do call set "NewCommand=%%NewCommand%% linuxBuild/%%a.o"
@echo on
%LINUX_ROOT%/bin/x86_64-unknown-linux-gnu-ar.exe cr linuxBuild/quat.a %NewCommand%