rem GUIA DE LECTURA NXK - COMPILAR-NXK.cmd
rem Invoca la construccion del kernel desde Windows.
rem Contexto y orden de estudio: ver LEER-CODIGO.md en la carpeta principal.
@echo off
cd /d "%~dp0"
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0build.ps1"
pause
