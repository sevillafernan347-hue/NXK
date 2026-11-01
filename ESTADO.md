# Estado de NXK 0.1

Entrega inicial: 31 de octubre de 2026.

- Kernel y cargador de arranque propios en C y ensamblador.
- Variante principal x86 de 32 bits, con cargador BIOS y entrada C propios. Probada en CPU `qemu32` con 32, 64, 128 y 1024 MiB. La variante x64 se conserva.
- Arranque BIOS x86-64 probado en QEMU con 64 MiB de RAM.
- Objetivo de equipos con 1 GB de RAM documentado en `COMPATIBILIDAD.md`. Pruebas de arranque superadas con 32, 64, 128 y 1024 MiB en QEMU; no equivalen a compatibilidad con una ASUS fisica.
- Salida VGA y serie, comandos `help`, `version`, `mem`, `halt` y teclado PS/2 basico por sondeo.
- Asignador incremental de memoria de laboratorio.
- Compilador portatil Zig/Clang y emulador QEMU incluidos.
- Visual Studio Code instalado para el usuario, con extensiones C/C++ y ensamblador.
- Tareas del editor para compilar, ejecutar y comprobar el arranque.
- Biblioteca original: 200 titulos, mayormente en ingles. La seleccion `Biblioteca/Espanol` contiene 10 titulos; completar 200 en espanol, con reparto por tema, sigue pendiente.
- Arranque, entrada, entrada de excepciones, llenado y copia de memoria x86 escritos en NEXTRY ASM. El resto todavia depende principalmente de C; no es un kernel completo en NEXTRY.
- NXK Espanol 0.1: compilador inicial, reglas, pruebas y fuentes `.nxf`.
- Formato ejecutable NXE 1 con cabecera propia, cargador y aplicacion nativa probada en x86 y x64.
- Puente Win32 experimental: un EXE PE32 propio probado en Windows y NXK x86, con GetStdHandle, WriteFile y ExitProcess. No hay compatibilidad general de aplicaciones Windows.

## Comprobaciones realizadas

`test.ps1` arranca una imagen especial y comprueba el marcador `NXK: BOOT PASS` y el codigo de salida del dispositivo de pruebas de QEMU. Se verifico tanto desde PowerShell 7 como desde Windows PowerShell 5, que utilizan los accesos `.cmd`.

Una prueba adicional envio pulsaciones PS/2 mediante el monitor de QEMU y comprobo que `help` devuelve la lista de comandos. Tambien se inspecciono visualmente la ventana del emulador con el arranque de NXK.

Los 200 archivos principales de la biblioteca se comprobaron contra sus hashes SHA-256 y contra el reparto de 20 titulos por tema. No se contaron duplicados binarios como libros distintos.

## Siguiente etapa

Mapa BIOS E820 agregado al arranque NEXTRY ASM x86. `kernel/ram.c` informa
RAM utilizable por debajo de 4 GiB; el comando `mem` la distingue del heap.
Pruebas de deteccion superadas en QEMU con 32, 64, 128 y 1024 MiB.
El mapa ya alimenta un gestor de paginas fisicas x86: reserva/liberacion de 4 KiB,
limite inicial 1 GiB, exclusion de regiones reservadas y proteccion de los primeros
8 MiB. Los metadatos usan 64 KiB en 0x600000, verificados contra el mapa BIOS.
El arranque x64 aun no proporciona el mapa ni habilita este gestor.
Pruebas de agotamiento y recuperacion de todas las paginas superadas en las cuatro
configuraciones x86; tambien solapamientos, doble liberacion y direcciones invalidas.
Cada pagina fisica ahora se limpia al reservarla; las pruebas comprueban que los
datos escritos antes de liberarla no aparezcan en su siguiente uso.
El gestor tambien puede excluir rangos de dispositivos o firmware, con pruebas
para rangos parciales, repetidos y que cruzan paginas prestadas.
Sigue pendiente memoria virtual y uso del gestor por tareas. El heap antiguo sigue
siendo incremental; no tiene liberacion. El nuevo gestor es un puente C pendiente
de migrar a NEXTRY. Diagnostico detallado de excepciones implementado en la rama de trabajo.
Lee `COLABORACION.md` para repartir tareas entre cuentas con copias separadas.

Se agregaron comentarios de lectura a los 53 archivos fuente y lanzadores propios.
Consulta `LEER-CODIGO.md` para recorrerlos en orden. `lib/mover32.nxa` implementa
movimiento con solapamiento; sus pruebas cubren 2112 combinaciones por arranque.
Los cargadores NXE y PE32 ahora usan nuestra copia de memoria x86.

Ya existe una IDT inicial x86 que detiene la CPU ante excepciones. Ahora muestra vector, codigo de error, EIP y registros; pruebas provocadas de INT3, instruccion invalida y proteccion general. El prototipo aun no tiene procesos, planificador, sistema de archivos, red, escritorio grafico o soporte para arrancar directamente en una laptop UEFI.

Ultima comprobacion: copia de memoria NEXTRY ASM con longitudes 0, 1, 3, 16, 17 y 256, direcciones desalineadas, retorno del destino, origen intacto y centinelas. Arranque y pruebas superadas en x86 con 32, 64, 128 y 1024 MiB y en x64 con 64 MiB. Esto no garantiza ausencia de errores.

Temporizador inicial x86: PIT ~100 Hz, PIC con IRQ0, contador consultable con
`tiempo`. Pruebas de cinco ticks y retorno IRQ superadas en las cuatro
configuraciones de RAM. Sigue sin planificador. Ver ENTREGA-RELOJ.md.

Entrada IRQ0 migrada a kernel/reloj32.nxa. El ensamblador propio ya permite
recuperar registros generales y retornar de una interrupcion x86.

Demostracion inicial de tareas cooperativas x86 disponible con `tareas`.
Dos pilas privadas, cuatro turnos por tarea y variables locales preservadas.
Cambio de contexto en NEXTRY ASM y organizacion en C; sin preempcion ni
aislamiento. Validacion: ENTREGA-TAREAS.md.

Integracion en main: diagnostico de excepciones, PIT, tareas cooperativas y
validacion PE. El gestor fisico ahora rechaza operaciones antes de inicializar
sus metadatos, evitando accesos a una zona RAM aun no validada. Prueba previa
al inicio y matriz x86 32/64/128/1024 MiB superadas.

Archivos RAM propios disponibles: crear/reemplazar, leer, listar y borrar.
Ocho entradas de 256 bytes, sin persistencia. Ver ENTREGA-ARCHIVOS-RAM.md.

Tareas cooperativas: estado DORMIDA y despertar por plazos PIT, reposo HLT
cuando ninguna esta lista y finalizacion al retornar del trabajo. Demo `tareas`
verificada en matriz x86; sigue sin preempcion ni API general de procesos.
