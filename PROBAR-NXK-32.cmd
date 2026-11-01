rem GUIA DE LECTURA NXK - PROBAR-NXK-32.cmd
rem Ejecuta las pruebas de arranque de NXK de 32 bits.
rem Contexto y orden de estudio: ver LEER-CODIGO.md en la carpeta principal.
@echo off
cd /d "%~dp0"
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0test.ps1" -Architecture x86 -MemoryMatrix
pause
