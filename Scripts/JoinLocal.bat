@echo off
rem Join a NULL-ACTION server running on THIS computer.
set UE=C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe
set PROJECT=%~dp0..\NullAction.uproject
set /p NAME=Your player name: 
"%UE%" "%PROJECT%" 127.0.0.1:7777?Name=%NAME% -game -windowed -ResX=1280 -ResY=720 -log
