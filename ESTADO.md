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

NEXTRY acepta tipos explicitos entero/logico/texto y constantes tipadas.
El modulo kernel/inicio.nxf ya usa declaraciones explicitas. 19 pruebas del
compilador, matriz x86 y regresion x64 superadas. entero sigue siendo uint32
sin signo; los tipos con signo, decimales y enteros largos no estan implementados.
Build y test aceptan -FuenteApp para integrar y probar aplicaciones propias
sin editar fuentes del kernel. Ejemplo tipos.nxf ejecutado en x86 con 64 MiB.
El alias de consola sigue siendo ejecutar hola.nxe. Imagen local x86 actual
contiene tipos.nxf como demostracion. El PDF anterior no incluye esta extension;
la referencia actualizada esta en lenguaje/REGLAS.md.

El gestor fisico x86 reserva y libera bloques contiguos con alineacion en
paginas. Validacion completa antes de liberar, borrado y pruebas negativas.
Ver ENTREGA-BLOQUES-MEMORIA.md. Sigue sin sincronizacion ni memoria virtual.

Paginacion inicial x86 activada: mapeo identidad 1 GiB, solo supervisor, pagina
cero ausente y codigo/rodata del kernel protegidos con CR0.WP. Diagnostico CR2
para fallos de pagina. No constituye aislamiento de procesos; ver
ENTREGA-PAGINACION.md para pruebas y limites.

Las pilas de las dos tareas x86 ahora tienen pagina de guarda ausente y
reservas reutilizables. Sin pila de emergencia aun; detalles y limites en
ENTREGA-PAGINACION.md.

Guardas: validacion de paginacion activa y heap reservado; contador observable
con mem y pruebas de idempotencia/reutilizacion. Lenguajes del repositorio
actualizados en .gitattributes, README y LENGUAJES.md.

Doble fallo x86: puerta de tarea y TSS de emergencia con pila independiente,
probadas provocando un fallo sobre la guarda con ESP inutilizable. El manejador
informa ESP anterior y detiene el kernel; no recupera procesos. Ver
ENTREGA-EMERGENCIA.md. Sustituye la limitacion previa de ausencia de pila
de emergencia para doble fallo.

Prueba inicial de modo usuario x86: segmentos DPL3, paginas usuario y pila SS0/ESP0. INT 0x80 autorizado; CLI y escritura sobre kernel rechazadas. Solo pruebas fatales; aplicaciones habituales siguen ring0. Ver ENTREGA-MODO-USUARIO.md.

Comando usuario x86: demostracion conocida ring3 termina con codigo 42 y devuelve control al kernel. Repetible sin consumo adicional de heap; permisos de paginas y puerta temporal restaurados. Sin preempcion ni recuperacion de fallos de usuario. Ver ENTREGA-MODO-USUARIO.md.

Recuperacion de la demostracion de usuario: cuatro fallos controlados vuelven al llamador y a consola; comando usuario fallos. Fallos ring0 y doble fallo siguen fatales. Matriz x86, diez escenarios anteriores, regresion x64 y consola automatizada superados. No es aun gestion general de procesos.

Diagnosticos de usuario: historial circular de ocho resultados (salida/vector/error/EIP/CR2), consultable con usuario estado e usuario historial. usuario nulo provoca y contiene un fallo conocido. Pruebas de limpieza, orden y limites del historial, matriz x86, x64 y consola superadas.

Puente de modo usuario migrado de GNU asm a tres fuentes NEXTRY ASM: entrada,
salida y aborto. Simbolos externos resueltos en dos pasadas y verificados.
Pruebas: ENTREGA-USUARIO-NXA.md. TSS/GDT y organizacion siguen en C.

La demostracion de usuario usa directorio y tabla inferior privados (CR3 separado) sin cambiar permisos del mapa kernel. Verifica restauracion y permisos originales tras cada salida. usuario fallos cubre seis casos, incluyendo escritura en tablas privadas y ESP usuario invalido; ambos contenidos. Historial incluye ESP. Matriz x86, x64 y consola superadas. Sigue siendo un unico espacio reutilizable, sin procesos generales ni preempcion. Ver ENTREGA-ESPACIO-USUARIO.md.

Proteccion de la demo: permisos usuario retirados por defecto de mapas heredados y habilitados solo en codigo/pila propios. Prueba normal usa su pila real; se agregan escritura sobre codigo propio y lectura del heap kernel. usuario fallos ejecuta ocho casos. Matriz x86, consola y regresion x64 superadas; aun sin NX ni procesos multiples.

Usuario con IRQ habilitada y limite PIT de diez ticks: usuario bucle recupera
consola sin cooperacion del programa. Sustituye la limitacion previa de IF
apagada durante esta demo. No es planificador preventivo entre procesos.
Construccion verifica bytes NXA enlazados y huellas de incbin. Ver
ENTREGA-LIMITE-USUARIO.md.

Primer servicio retornable usuario INT 0x81: salida de hasta 256 bytes con
validacion del rango y copia kernel. Comando usuario texto. Pruebas positivas,
punteros/longitudes invalidos y llamada desconocida. Cargador x86 ampliado a
48 KiB de imagen; x64 sigue 32 KiB. Ver ENTREGA-SERVICIO-CONSOLA.md.

NEXTRY CPL3: usuario nextry ejecuta fuente .nxf integrada con runtime de usuario,
texto y numeros mediante servicios validados. FuenteUsuario permite elegir
archivo al construir. Codigo de una pagina y pila propia; no cambia NXE ni
carga archivos externos. Ver ENTREGA-NEXTRY-USUARIO.md.

Aplicaciones NEXTRY protegidas pueden guardar, mostrar y borrar archivos RAM
mediante servicios validados. Destinos de lectura limitados a pila escribible.
Compilador con 20 pruebas y ejemplo completo; ver ENTREGA-ARCHIVOS-USUARIO.md.

Controlador ATA PIO de lectura x86: identifica IDE primario y lee sectores
LBA28 con limites, sondeos acotados y buffer temporal. Comando discos.
Validacion en QEMU: ENTREGA-ATA-LECTURA.md. Sin escrituras ni persistencia.

Volumen NXFS1 de solo lectura sobre imagen IDE secundaria: montaje, listado y
lectura hasta 4096 bytes con verificacion de huellas y metadatos. Comandos
volumen montar/listar/leer. Nueve imagenes de prueba superadas. Aun sin guardar
desde NXK al disco. Ver ENTREGA-VOLUMEN-NXFS.md.
