@echo off

rem Initial setup

rem Check if Unreal Editor is running
tasklist /fi "IMAGENAME eq UnrealEditor.exe" 2>NUL | find /i /n "UnrealEditor.exe" >NUL

rem No error if it running, applies force exit 
if %errorlevel% == 0 (
    echo Error: Unreal Editor is running.
    echo Close Unreal Editor before building the project.
    pause
    exit /b 0
)

set G_PROJECT_PATH="%~dp0.."

@echo Cleaning up...

set L_SAVED_FOLDER_PATH=%G_PROJECT_PATH%\Saved
set L_CONFIG_FOLDER_PATH=%L_SAVED_FOLDER_PATH%\Config

rem Start script

if exist %G_PROJECT_PATH%\Binaries (
    rd /q /s %G_PROJECT_PATH%\Binaries
)
if exist %G_PROJECT_PATH%\DerivedDataCache (
    rd /q /s %G_PROJECT_PATH%\DerivedDataCache
)
if exist %G_PROJECT_PATH%\Intermediate (
    rd /q /s %G_PROJECT_PATH%\Intermediate
)
for /d %%i in ("%L_SAVED_FOLDER_PATH%\*") do (
    if "%%i" NEQ "%L_CONFIG_FOLDER_PATH%" (
        rd /q /s "%%i"
    )
)

git clean -dfx Plugins/

rem End script