# Puente experimental de Windows para NXK

La vía principal de aplicaciones de NXK es **NXF → NXE**. El puente de Windows es una vía complementaria para avanzar hacia la migración gradual.

## Lo que ya funciona

En NXK de 32 bits se carga y ejecuta `hola.exe`, un archivo PE32 Win32 construido desde `hola.c`. El mismo archivo se probó en Windows y dentro de NXK, con código de salida cero. Imprime un texto mediante las importaciones reales `GetStdHandle`, `WriteFile` y `ExitProcess` de `KERNEL32.dll`.

En NXK esas tres importaciones se enlazan con implementaciones mínimas propias. No se copia ni se carga la DLL de Microsoft. `WriteFile` solo atiende salida y error estándar síncronos, con buffers pequeños; `ExitProcess` devuelve el control al cargador mediante un puente ensamblador. No hay procesos o hilos de Windows reales.

En la consola de NXK de 32 bits escribe:

```text
ejecutar hola.exe
```

El archivo se incorpora al disco virtual durante la construcción. Aún no se pueden abrir rutas arbitrarias de EXE desde USB o disco.

## Límites exactos

- Solo PE32 x86 de consola, con base `0x400000`, hasta 128 KiB de imagen y ocho secciones.
- Solo esas tres funciones de `KERNEL32.dll`. Otras DLL, importaciones por ordinal, TLS, CLR/.NET e importaciones diferidas se rechazan.
- No se aplica relocalización: la imagen debe poder usar la base fija indicada.
- No se implementan archivos, interfaz gráfica, registro, red, hilos, SEH, controladores, DirectX ni las demás API de Windows.
- El puente no ejecuta PE32 en la variante x64 de NXK.
- La aplicación corre con privilegios del kernel y comparte su pila. Las comprobaciones del formato no aíslan instrucciones arbitrarias.

Esto es una demostración de compatibilidad binaria muy limitada. No implica que Office, navegadores, instaladores, juegos ni aplicaciones habituales de Windows funcionen. No se ha añadido un conversor automático EXE → NXE.

## Ruta para ampliar compatibilidad

1. IDT, excepciones, procesos aislados y memoria virtual.
2. Sistema de archivos y cargador que pueda abrir ejecutables externos.
3. Ampliar y comprobar las API que necesite cada aplicación objetivo.
4. Bibliotecas, relocalizaciones, entorno de procesos e hilos.
5. Interfaz gráfica y subsistemas adicionales según los programas elegidos.

La compatibilidad se medirá por aplicaciones y funciones verificadas. Un programa futuro puede reescribirse en NXK Español y pasar a NXE; otro podrá conservar su EXE si el puente implementa sus dependencias.

## Documentación primaria utilizada

- [Formato PE/COFF de Microsoft](https://learn.microsoft.com/en-us/windows/win32/debug/pe-format).
- [GetStdHandle](https://learn.microsoft.com/en-us/windows/console/getstdhandle).
- [WriteFile](https://learn.microsoft.com/en-us/windows/win32/api/fileapi/nf-fileapi-writefile).
- [ExitProcess](https://learn.microsoft.com/en-us/windows/win32/api/processthreadsapi/nf-processthreadsapi-exitprocess).
