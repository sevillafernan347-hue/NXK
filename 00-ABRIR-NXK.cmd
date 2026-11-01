rem GUIA DE LECTURA NXK - 00-ABRIR-NXK.cmd
rem Menu principal para leer, programar, abrir la biblioteca y arrancar NXK.
rem Contexto y orden de estudio: ver LEER-CODIGO.md en la carpeta principal.
@echo off
cd /d "%~dp0"
:menu
cls
echo NXK - TODO EL PROYECTO
echo.
echo 1. Leer que tenemos, que falta y por donde vamos
echo 2. Abrir el proyecto en Visual Studio Code
echo 3. Encender NXK de 32 bits en QEMU
echo 4. Programar en NXJ
echo 5. Programar en NXK Espanol
echo 6. Abrir la biblioteca de 200 titulos
echo 7. Abrir esta carpeta principal
echo 0. Salir
echo.
choice /c 12345670 /n /m "Elige una opcion: "
if errorlevel 8 exit /b
if errorlevel 7 (start "" explorer.exe "%~dp0" & goto menu)
if errorlevel 6 (start "" explorer.exe "%~dp0Biblioteca" & goto menu)
if errorlevel 5 (call "%~dp0EDITAR-APP.cmd" & goto menu)
if errorlevel 4 (call "%~dp0PROGRAMAR-NXJ.cmd" & goto menu)
if errorlevel 3 (start "" "%~dp0INICIAR-NXK-32.cmd" & goto menu)
if errorlevel 2 (call "%~dp0ABRIR-PROYECTO.cmd" & goto menu)
if errorlevel 1 (start "" notepad.exe "%~dp000-LEER-PRIMERO.txt" & goto menu)
