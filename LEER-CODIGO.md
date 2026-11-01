# Leer el codigo de NXK

Empieza por `kernel/inicio.nxf`: es el ejemplo mas corto de nuestro lenguaje.
Despues lee `boot/arranque32.nxa`, `kernel/entrada32.nxa` y `kernel/main.c`, en ese orden.
El BIOS carga el arranque, este carga el kernel, la entrada limpia sus variables y main inicia la consola.

## Como interpretar los archivos

- `.nxf`: NEXTRY; `funcion` abre una funcion y `fin` cierra un bloque. `#` inicia comentarios.
- `.nxa`: nuestro ensamblador; las etiquetas terminan en `:`. EAX, ECX, ESI y EDI son registros de la CPU.
- `.nxj`: sintaxis experimental inspirada en Java, con capacidades limitadas.
- `.c` y `.h`: implementacion C e interfaces que todavia faltan migrar. `/* ... */` encierra comentarios.
- `.S`: ensamblador externo, algunas fuentes son referencias historicas y otras siguen activas.
- `.py`: compilador, ensamblador y pruebas que corren en Windows, no dentro de NXK.
- `.ps1` y `.cmd`: herramientas para construir y abrir el proyecto desde Windows.
- `.ld`: reglas que colocan el codigo y los datos en direcciones concretas.

No edites `build` para cambiar el programa: su contenido se genera otra vez al compilar.
`tools` contiene dependencias externas; `Biblioteca` contiene material de estudio.

## Mapa de los archivos propios

- [kernel/ram.c](kernel/ram.c): interpreta el mapa BIOS recibido del arranque x86; puente C pendiente de migracion.
- [COLABORACION.md](COLABORACION.md): instrucciones y mensajes para las otras cuentas.

- [00-ABRIR-NXK.cmd](00-ABRIR-NXK.cmd): Menu principal para leer, programar, abrir la biblioteca y arrancar NXK.
- [ABRIR-PROYECTO.cmd](ABRIR-PROYECTO.cmd): Abre la carpeta de NXK en Visual Studio Code.
- [COMPILAR-NXK.cmd](COMPILAR-NXK.cmd): Invoca la construccion del kernel desde Windows.
- [CREAR-APP-NXE.cmd](CREAR-APP-NXE.cmd): Compila el ejemplo NEXTRY como ejecutable NXE.
- [CREAR-APP-NXJ.cmd](CREAR-APP-NXJ.cmd): Compila el ejemplo NXJ usando la interfaz experimental de clases.
- [EDITAR-APP.cmd](EDITAR-APP.cmd): Abre el ejemplo NEXTRY para editarlo.
- [INICIAR-NXK-32.cmd](INICIAR-NXK-32.cmd): Abre la maquina virtual con NXK de 32 bits.
- [INICIAR-NXK.cmd](INICIAR-NXK.cmd): Abre la maquina virtual con el lanzador configurado.
- [PROBAR-LENGUAJE.cmd](PROBAR-LENGUAJE.cmd): Ejecuta las comprobaciones del compilador del lenguaje.
- [PROBAR-NXK-32.cmd](PROBAR-NXK-32.cmd): Ejecuta las pruebas de arranque de NXK de 32 bits.
- [PROBAR-RAM.cmd](PROBAR-RAM.cmd): Ejecuta pruebas del kernel con varias cantidades de RAM virtual.
- [PROGRAMAR-NXJ.cmd](PROGRAMAR-NXJ.cmd): Abre el ejemplo NXJ para programar en el editor.
- [boot/arranque32.nxa](boot/arranque32.nxa): Arranque BIOS usado en 32 bits: lee el kernel del disco, prepara segmentos y activa el modo protegido.
- [boot/boot.S](boot/boot.S): Arranque BIOS de la variante de 64 bits: prepara la CPU y transfiere el control al kernel.
- [boot/boot.ld](boot/boot.ld): Reglas de enlace del sector de arranque de referencia: ubican el codigo para el BIOS.
- [boot/boot32.S](boot/boot32.S): Referencia antigua del arranque de 32 bits. La construccion actual usa arranque32.nxa.
- [build.ps1](build.ps1): Construye el kernel: compila NEXTRY, ensambla NXA, enlaza modulos y crea la imagen de disco.
- [compatibilidad/windows/hola.c](compatibilidad/windows/hola.c): Programa Windows de demostracion que usa solo tres funciones Win32 admitidas por NXK.
- [compatibilidad/windows/pe.c](compatibilidad/windows/pe.c): Valida PE32, carga secciones y conecta tres funciones Win32. No ejecuta apps Windows generales.
- [compatibilidad/windows/retorno32.S](compatibilidad/windows/retorno32.S): Guarda la pila del kernel al entrar al EXE y la recupera cuando este termina.
- [compilar-app.ps1](compilar-app.ps1): Transforma una app NEXTRY o NXJ en codigo nativo y la empaqueta como archivo NXE.
- [kernel/entrada32.nxa](kernel/entrada32.nxa): Entrada actual de 32 bits: pone a cero variables BSS y llama a kernel_main.
- [kernel/entry.S](kernel/entry.S): Entrada de 64 bits: prepara el entorno inicial y llama al kernel en C.
- [kernel/entry32.S](kernel/entry32.S): Referencia antigua de la entrada de 32 bits, sustituida por entrada32.nxa.
- [kernel/excepciones.c](kernel/excepciones.c): Construye la IDT: tabla que indica a la CPU donde ir ante una excepcion. Solo implementada para x86.
- [kernel/excepciones32.S](kernel/excepciones32.S): Referencia anterior del manejador de excepciones, sustituida por excepciones32.nxa.
- [kernel/excepciones32.nxa](kernel/excepciones32.nxa): Entrada comun ante una excepcion x86: guarda registros y llama al manejador fatal; no regresa.
- [kernel/inicio.nxf](kernel/inicio.nxf): Modulo del kernel en NEXTRY: bienvenida y ejemplos comprobados de variables, ciclos y condiciones.
- [kernel/linker.ld](kernel/linker.ld): Distribuye codigo, datos y variables del kernel en memoria y verifica limites de tamano.
- [kernel/main.c](kernel/main.c): Coordina el kernel: consola VGA/serie, teclado, servicios para NEXTRY, pruebas y comandos.
- [kernel/nxe.c](kernel/nxe.c): Valida y ejecuta nuestro formato NXE en una direccion fija. Las apps comparten privilegios con el kernel.
- [kernel/pruebas_cargadores.c](kernel/pruebas_cargadores.c): Pruebas negativas: construye archivos incorrectos para comprobar que los cargadores los rechacen.
- [lenguaje/aplicacion.ld](lenguaje/aplicacion.ld): Reglas de enlace de apps NXE: ubicacion fija, punto de entrada y limite de memoria.
- [lenguaje/compilador.py](lenguaje/compilador.py): Compilador NEXTRY: separa palabras, analiza estructura y tipos, y produce C sin sistema operativo anfitrion.
- [lenguaje/ejemplos/hola.nxf](lenguaje/ejemplos/hola.nxf): Ejemplo de aplicacion NEXTRY compilada y empaquetada como hola.nxe.
- [lenguaje/ejemplos/hola.nxj](lenguaje/ejemplos/hola.nxj): Ejemplo de la sintaxis experimental de clases y metodos estaticos NXJ.
- [lenguaje/empaquetar.py](lenguaje/empaquetar.py): Crea la cabecera NXE con arquitectura, tamanos, entrada y suma de comprobacion del programa.
- [lenguaje/ensamblador.py](lenguaje/ensamblador.py): Ensamblador propio: convierte instrucciones espanolas en bytes x86; dos pasadas resuelven etiquetas.
- [lenguaje/nxj.py](lenguaje/nxj.py): Interfaz experimental inspirada en Java: transforma clases con metodos estaticos al lenguaje base; no es Java completo.
- [lenguaje/pruebas.py](lenguaje/pruebas.py): Pruebas del compilador: comprueban programas validos y errores de sintaxis, tipos y limites.
- [lenguaje/pruebas_ensamblador.py](lenguaje/pruebas_ensamblador.py): Comprueba los bytes de instrucciones, desplazamientos de saltos y rechazo de operandos incorrectos.
- [lenguaje/runtime_nxe.c](lenguaje/runtime_nxe.c): Adaptador de la app NXE: recibe la tabla de servicios del kernel y llama al inicio de NEXTRY.
- [lenguaje/simbolos_elf.py](lenguaje/simbolos_elf.py): Lee direcciones del archivo ELF enlazado para resolver referencias externas de la entrada NEXTRY ASM.
- [lib/copiar32.nxa](lib/copiar32.nxa): Implementa memcpy en NEXTRY ASM: copia bytes entre regiones que no se solapan.
- [lib/llenar32.nxa](lib/llenar32.nxa): Implementa memset en NEXTRY ASM: llena una region con un byte y devuelve su direccion.
- [lib/memory.c](lib/memory.c): Asignador incremental de 512 KiB y comparacion de cadenas. Incluye rutinas de memoria C para x64.
- [lib/mover32.nxa](lib/mover32.nxa): Implementa memmove en NEXTRY ASM: permite copiar incluso cuando las regiones se superponen.
- [lib/nxe.h](lib/nxe.h): Contrato binario de servicios NXE: version y punteros a funciones que recibe una app nativa.
- [lib/nxk.h](lib/nxk.h): Declaraciones compartidas: reserva de memoria, operaciones de bytes y comparacion de texto.
- [lib/nxk_es.h](lib/nxk_es.h): Interfaz entre el codigo generado desde NEXTRY y los servicios del kernel.
- [lib/pe.h](lib/pe.h): Interfaz publica del cargador experimental de programas Windows PE32.
- [run.ps1](run.ps1): Arranca QEMU con una copia de la imagen para poder reconstruir mientras la maquina virtual esta abierta.
- [test.ps1](test.ps1): Construye una imagen de pruebas, arranca QEMU, verifica resultados y reconstruye la imagen normal.

## Avance actual

El movimiento de memoria con solapamiento ya esta en NEXTRY ASM. Los cargadores NXE y PE32 usan la copia propia en x86.
Faltan procesos, planificador, archivos, controladores y migrar el resto del kernel. NEXTRY aun genera C.
La biblioteca espanola tiene 10 titulos seleccionados; llegar a 200 en espanol sigue pendiente.
