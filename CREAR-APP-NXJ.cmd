rem GUIA DE LECTURA NXK - CREAR-APP-NXJ.cmd
rem Compila el ejemplo NXJ usando la interfaz experimental de clases.
rem Contexto y orden de estudio: ver LEER-CODIGO.md en la carpeta principal.
@echo off
cd /d "%~dp0"
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0compilar-app.ps1" -Fuente lenguaje/ejemplos/hola.nxj -Salida build/x86/hola-java.nxe
pause
