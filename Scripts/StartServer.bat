@echo off
rem NULL-ACTION local dedicated server (no window, server-authoritative, port 7777).
rem Needs the level Content\NullAction\Maps\L_TestArena (File > New Level > Empty Level, save it there).
set UE=C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe
set PROJECT=%~dp0..\NullAction.uproject
echo Starting NULL-ACTION dedicated server on port 7777...
"%UE%" "%PROJECT%" /Game/NullAction/Maps/L_TestArena -server -log -port=7777
