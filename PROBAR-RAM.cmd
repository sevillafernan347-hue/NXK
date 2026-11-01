rem GUIA DE LECTURA NXK - PROBAR-RAM.cmd
rem Ejecuta pruebas del kernel con varias cantidades de RAM virtual.
rem Contexto y orden de estudio: ver LEER-CODIGO.md en la carpeta principal.
@echo off
cd /d "%~dp0"
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0test.ps1" -MemoryMatrix
pause
