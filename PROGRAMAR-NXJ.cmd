rem GUIA DE LECTURA NXK - PROGRAMAR-NXJ.cmd
rem Abre el ejemplo NXJ para programar en el editor.
rem Contexto y orden de estudio: ver LEER-CODIGO.md en la carpeta principal.
@echo off
set "NXK_CODE=%LOCALAPPDATA%\Programs\Microsoft VS Code\Code.exe"
if exist "%NXK_CODE%" (
  start "" "%NXK_CODE%" "%~dp0" "%~dp0lenguaje\ejemplos\hola.nxj"
) else (
  echo Abre lenguaje\ejemplos\hola.nxj en Visual Studio Code.
  pause
)
