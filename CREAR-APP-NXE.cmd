rem GUIA DE LECTURA NXK - CREAR-APP-NXE.cmd
rem Compila el ejemplo NEXTRY como ejecutable NXE.
rem Contexto y orden de estudio: ver LEER-CODIGO.md en la carpeta principal.
@echo off
cd /d "%~dp0"
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0compilar-app.ps1"
pause
