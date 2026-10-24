@echo off
rem Runic Wheel LE v0.26 - native Win32 build with the Microsoft C++ compiler (x86).
rem   set SKSE_LE_SDK=<folder that holds skse\ and common\ of the SKSE 1.7.3 sources>
rem   set VCVARS32=<path to vcvars32.bat>   (optional, found with vswhere otherwise)
rem   build-v026.bat [output folder]
setlocal
if "%SKSE_LE_SDK%"=="" (echo Set SKSE_LE_SDK to the SKSE LE 1.7.3 source folder first.& exit /b 1)
if "%VCVARS32%"=="" for /f "usebackq delims=" %%i in (`"%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe" -latest -products * -property installationPath`) do set "VCVARS32=%%i\VC\Auxiliary\Build\vcvars32.bat"
if not exist "%VCVARS32%" (echo vcvars32.bat not found: %VCVARS32%& exit /b 1)
call "%VCVARS32%" >nul 2>nul
set "OUT=%~1"
if "%OUT%"=="" set "OUT=%~dp0out"
if not exist "%OUT%" mkdir "%OUT%"
rem /MT: static CRT, so nothing to install. /EHa: real SEH frames for every __try guard.
rem /Zc:threadSafeInit-: no hidden guard on function-local statics (all are constant here).
rem /Brepro: no timestamp, so the same source gives the same DLL bytes.
cl /nologo /Brepro /c /O1 /EHa /GR- /W4 /MT /Zc:threadSafeInit- /I"%SKSE_LE_SDK%" /I"%SKSE_LE_SDK%\skse" /Fo"%OUT%\RunicWheelLE.obj" "%~dp0RunicWheelV011.cpp"
if errorlevel 1 exit /b 1
link /nologo /Brepro /dll /machine:x86 /release /opt:ref /opt:icf /out:"%OUT%\RunicWheelLE.dll" "%OUT%\RunicWheelLE.obj" kernel32.lib user32.lib
if errorlevel 1 exit /b 1
echo BUILD OK: %OUT%\RunicWheelLE.dll
