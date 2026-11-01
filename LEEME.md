# NXK 0.1

Proyecto de kernel original para aprender a crear un sistema operativo. No contiene codigo de Linux. El nucleo y el cargador se escribieron para este proyecto; Zig/LLVM y QEMU son herramientas externas con sus propias licencias.

La variante principal es **x86 de 32 bits**, orientada al objetivo de una ASUS antigua con 1 GB de RAM. Se conserva la variante x64; para elegirla pasa `-Architecture x64` a los scripts de compilacion, prueba o ejecucion.

## Lenguaje y aplicaciones propios

**NXK Español 0.1** usa fuentes `.nxf`. Consulta `lenguaje/REGLAS.md`. `EDITAR-APP.cmd` abre el ejemplo; `CREAR-APP-NXE.cmd` genera un ejecutable nativo `.nxe` con formato propio. `COMPILAR-NXK.cmd` reconstruye el nucleo e incorpora el ejemplo para poder ejecutarlo en la consola con `ejecutar hola.nxe`.

En NXK de 32 bits, `ejecutar hola.exe` prueba un puente Win32 que solo admite tres funciones de consola/salida. No es compatibilidad general con aplicaciones Windows; consulta `compatibilidad/windows/LEEME.md`. Ambos ejemplos estan embebidos durante la construccion: aun no se abren ejecutables arbitrarios desde disco.

## Abrir

1. Ejecuta `ABRIR-PROYECTO.cmd` para abrir NXK en Visual Studio Code.
2. Pulsa Ctrl+Shift+B para compilar. Tambien puedes usar `COMPILAR-NXK.cmd`.
3. En Terminal > Ejecutar tarea, elige `NXK: Ejecutar en QEMU`. Tambien puedes usar `INICIAR-NXK.cmd`.
4. La carpeta `Biblioteca` guarda los libros para leer localmente; su catalogo es `CATALOGO.md`.

`NXK: Probar arranque` ejecuta una verificacion automatica en QEMU. El editor no incluye compilador: el proyecto trae Zig/Clang y QEMU en `tools`.

La consola acepta `help`, `version`, `mem` y `halt`. Haz clic en la ventana QEMU y escribe con letras minusculas. Ctrl+Alt+G libera el teclado y el raton. Cierra la ventana para salir. No se escribe en discos fisicos.

## Que incluye

- Cargadores BIOS propios de 512 bytes: x86 entra en modo protegido de 32 bits; x64 configura paginacion y entra en modo de 64 bits.
- Nucleo C independiente de un sistema anfitrion, con salida VGA y puerto serie.
- Biblioteca propia inicial `lib/`: comparacion de cadenas, relleno de memoria y asignador incremental de laboratorio.
- Heap fijo de 512 KiB dentro de la memoria mapeada. No descubre la RAM real ni permite liberar bloques.
- Consola VGA y serie con entrada basica PS/2 por sondeo, sin Shift ni teclas internacionales.

## Limites actuales

Es un prototipo educativo, aun no un SO para uso diario. No tiene IDT ni manejo de excepciones, planificador, procesos de usuario, archivos, red, USB, escritorio grafico o soporte UEFI. Las interrupciones quedan deshabilitadas. Ambos cargadores requieren BIOS con lectura extendida de disco y un kernel de hasta 32 KiB. La variante x86 no necesita modo largo ni PAE y aun no activa paginacion. La variante x64 requiere una CPU con soporte de 64 bits. Las pruebas de x86 abarcan 32, 64, 128 y 1024 MiB de RAM virtual.

## Ruta de trabajo

1. Arranque de 32 y 64 bits y consola (esta entrega).
2. IDT, excepciones y mensajes de error controlados.
3. Mapa de memoria del firmware y asignador de paginas.
4. Temporizador, interrupciones y entrada de teclado.
5. Tareas, cambio de contexto y procesos aislados.
6. Llamadas al sistema, archivos y programas de usuario.

Cada etapa debe tener una prueba reproducible antes de avanzar.

## Equipos con 1 GB de RAM

Es un objetivo del proyecto. Consulta `COMPATIBILIDAD.md`. Ejecuta `PROBAR-RAM.cmd` para comprobar el arranque con 32, 64, 128 y 1024 MiB en QEMU. El modelo exacto de la ASUS y su compatibilidad fisica siguen pendientes de verificar.

## Construccion

`build.ps1` usa Zig 0.15.2 como distribucion portatil de Clang/LLD, compilando C y ensamblador hacia un destino freestanding. No usa bibliotecas de Linux o Windows dentro de NXK. Los archivos de 32 bits se guardan en `build/x86/`: `kernel.elf` conserva simbolos para depuracion y `nxk.img` es el disco virtual. Los archivos de la variante x64 se guardan directamente en `build/`.

Los manuales de otros sistemas en Biblioteca son material de estudio; su codigo no se incorpora automaticamente al proyecto.
