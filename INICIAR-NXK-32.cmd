rem GUIA DE LECTURA NXK - INICIAR-NXK-32.cmd
rem Abre la maquina virtual con NXK de 32 bits.
rem Contexto y orden de estudio: ver LEER-CODIGO.md en la carpeta principal.
@echo off
cd /d "%~dp0"
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0run.ps1" -Architecture x86 -MemoryMiB 64
pause
