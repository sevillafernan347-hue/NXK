# NXK y NEXTRY

Kernel original experimental para x86 de 32 bits, con objetivo de equipos de
1 GB de RAM. Incluye lenguaje NEXTRY, ensamblador NEXTRY ASM y formato NXE.

Empieza por [LEER-CODIGO.md](LEER-CODIGO.md), [ESTADO.md](ESTADO.md) y
[COLABORACION.md](COLABORACION.md). El codigo tiene comentarios en espanol.
Las propuestas de cambios se reciben mediante issues y pull requests; consulta
[CONTRIBUIR.md](CONTRIBUIR.md).

## Construir en Windows

Las dependencias externas no se incluyen en Git. Consulta [HERRAMIENTAS.md](HERRAMIENTAS.md)
para versiones y origen: Zig en `tools/zig`, Python en `tools/python` y QEMU
en `tools/qemu`. La laptop original ya las tiene. No copies credenciales al repositorio.

```powershell
powershell -ExecutionPolicy Bypass -File build.ps1 -Architecture x86
powershell -ExecutionPolicy Bypass -File test.ps1 -Architecture x86 -MemoryMatrix
powershell -ExecutionPolicy Bypass -File run.ps1 -Architecture x86
```

El mapa BIOS E820 se recoge en NEXTRY ASM y se consulta mediante un puente C.
La consola informa RAM descrita por BIOS y espacio del heap por separado.
El gestor x86 ya reserva y libera paginas fisicas de 4 KiB hasta 1 GiB.
Cada pagina se pone en cero antes de entregarla a un nuevo usuario.
Todavia faltan memoria virtual, procesos, archivos y aislamiento.
El compilador genera C; el kernel aun no esta escrito completamente en NEXTRY.
El EXE de demostracion usa solo tres funciones Win32: no hay compatibilidad general.

`Biblioteca`, herramientas y archivos generados se conservan localmente y no
forman parte de este repositorio. No se ha definido una licencia de redistribucion
del codigo propio. Las dependencias conservan sus licencias originales.
