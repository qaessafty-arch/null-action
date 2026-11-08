@echo off
rem Join a NULL-ACTION server on another computer (LAN IP like 192.168.1.20, or a public IP).
set UE=C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe
set PROJECT=%~dp0..\NullAction.uproject
set /p IP=Server IP address: 
set /p NAME=Your player name: 
"%UE%" "%PROJECT%" %IP%:7777?Name=%NAME% -game -windowed -ResX=1280 -ResY=720 -log
