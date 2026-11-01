rem GUIA DE LECTURA NXK - INICIAR-NXK.cmd
rem Abre la maquina virtual con el lanzador configurado.
rem Contexto y orden de estudio: ver LEER-CODIGO.md en la carpeta principal.
@echo off
cd /d "%~dp0"
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0run.ps1"
pause
