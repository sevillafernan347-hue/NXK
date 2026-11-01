rem GUIA DE LECTURA NXK - ABRIR-PROYECTO.cmd
rem Abre la carpeta de NXK en Visual Studio Code.
rem Contexto y orden de estudio: ver LEER-CODIGO.md en la carpeta principal.
@echo off
set "NXK_CODE=%LOCALAPPDATA%\Programs\Microsoft VS Code\Code.exe"
if exist "%NXK_CODE%" (
  start "" "%NXK_CODE%" "%~dp0" "%~dp0LEEME.md"
) else (
  echo Abre Visual Studio Code y elige Archivo - Abrir carpeta - NXK.
  pause
)
