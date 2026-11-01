rem GUIA DE LECTURA NXK - EDITAR-APP.cmd
rem Abre el ejemplo NEXTRY para editarlo.
rem Contexto y orden de estudio: ver LEER-CODIGO.md en la carpeta principal.
@echo off
set "NXK_CODE=%LOCALAPPDATA%\Programs\Microsoft VS Code\Code.exe"
if exist "%NXK_CODE%" (
  start "" "%NXK_CODE%" "%~dp0" "%~dp0lenguaje\ejemplos\hola.nxf"
) else (
  echo Abre lenguaje\ejemplos\hola.nxf en tu editor.
  pause
)
