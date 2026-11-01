# NXK y NEXTRY

Kernel original experimental para x86 de 32 bits, con objetivo de equipos de
1 GB de RAM. Incluye lenguaje NEXTRY, ensamblador NEXTRY ASM y formato NXE.

Empieza por [LEER-CODIGO.md](LEER-CODIGO.md), [ESTADO.md](ESTADO.md) y
[COLABORACION.md](COLABORACION.md). El codigo tiene comentarios en espanol.
Las propuestas de cambios se reciben mediante issues y pull requests; consulta
[CONTRIBUIR.md](CONTRIBUIR.md).

## Lenguajes utilizados

NEXTRY (`.nxf`), NXJ experimental (`.nxj`) y NEXTRY ASM (`.nxa`) son propios.
El kernel conserva puentes C; los compiladores usan Python y la construccion,
PowerShell y Batch. Consulta el [inventario de fuentes](LENGUAJES.md).
GitHub no admite inventar categorias nuevas mediante .gitattributes: NEXTRY
se documenta aqui; NEXTRY ASM se agrupa como Assembly.

## Aplicacion NEXTRY en modo usuario

`lenguaje/ejemplos/usuario.nxf` ya ejecuta texto, numeros y afirmaciones en CPL3.
En una nueva imagen, usa `usuario nextry`. Para integrar tu propia fuente:
`./build.ps1 -FuenteUsuario mi_programa.nxf`. En VS Code tambien existe la tarea
«NXK: Integrar aplicacion NEXTRY protegida actual» para el archivo abierto.
La app sigue integrada en la imagen, con codigo limitado a 4 KiB y runtime
inicial; consulta [la entrega](ENTREGA-NEXTRY-USUARIO.md).

## Aplicaciones NXE2 desde disco

Ya se puede compilar una fuente NEXTRY a NXE version 2 e incluirla en NXFS,
independientemente del kernel. Tras montar el volumen, usa
`ejecutar usuario disco.nxe`. Consulta [el formato y los pasos](lenguaje/FORMATO-NXE-USUARIO.md).
NXE version 1 conserva su ruta anterior en ring0; no se cambia silenciosamente.

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
Ya hay paginacion inicial y archivos RAM. Faltan espacios por proceso,
archivos persistentes y aislamiento.
El compilador genera C; el kernel aun no esta escrito completamente en NEXTRY.
El EXE de demostracion usa solo tres funciones Win32: no hay compatibilidad general.

`Biblioteca`, herramientas y archivos generados se conservan localmente y no
forman parte de este repositorio. No se ha definido una licencia de redistribucion
del codigo propio. Las dependencias conservan sus licencias originales.
