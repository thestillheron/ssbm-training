@echo off
setlocal

REM Drag a vanilla Melee v1.02 NTSC ISO onto this file to make rtc-training.iso
REM next to it. Your ISO is only read, never changed. Modelled on TM-CE's script.

set ISO="%~1"

if %ISO%=="" (
    echo Please drag a v1.02 NTSC melee iso on top of 'DRAG VANILLA MELEE HERE.bat'
    goto end
) else (
    echo iso: %ISO%
)

"%~dp0xdelta3.exe" -f -d -s %ISO% "%~dp0patch.xdelta" "%~dp0rtc-training.iso" || ( del "%~dp0rtc-training.iso" 2>nul & echo ERROR: The ISO is not a valid v1.02 NTSC melee iso & goto end )
echo rtc-training.iso has been successfully created!

:end

REM pause if not run from command line
echo %CMDCMDLINE% | findstr /C:"/c">nul && pause
