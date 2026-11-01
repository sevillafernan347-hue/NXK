rem GUIA DE LECTURA NXK - PROBAR-LENGUAJE.cmd
rem Ejecuta las comprobaciones del compilador del lenguaje.
rem Contexto y orden de estudio: ver LEER-CODIGO.md en la carpeta principal.
@echo off
cd /d "%~dp0"
"%~dp0tools\python\python.exe" "%~dp0lenguaje\pruebas.py"
pause
