# NEXTRY: lenguaje fundamental de NXK

## Requisito principal del usuario

Todo el sistema operativo NXK y todo su kernel deben estar escritos y basados en el lenguaje propio **NEXTRY**. No basta con ofrecer un lenguaje para aplicaciones sobre un kernel escrito en C o ensamblador.

NEXTRY es el nombre definitivo del lenguaje fundamental. Las denominaciones anteriores «NXK Espanol» y «NXJ» describen los prototipos realizados durante la exploracion. Sus componentes pueden reutilizarse para construir NEXTRY; no son dos fundamentos independientes del sistema.

Los objetivos de comodidad y capacidades inspiradas en Java, incluidos objetos e hilos, pasan al desarrollo de NEXTRY. No se afirma compatibilidad actual con Java.

## Alcance

- Arranque y entrada al kernel.
- Memoria, interrupciones, tareas, procesos y controladores.
- Cargadores NXE y capa de compatibilidad Windows.
- Servicios, bibliotecas, interfaz y aplicaciones propias del sistema.
- Compilador y herramientas propias como parte del objetivo de autoalojamiento.

Las instrucciones de procesador necesarias deben expresarse mediante construcciones o intrinsecos definidos por NEXTRY y generados por su compilador. Cambiar la extension de codigo C o ensamblador no constituye una migracion.

## Estado real

El prototipo actual NO cumple aun este requisito. Su nucleo y arranque conservan C y ensamblador; el compilador inicial usa Python y genera C. Los ejemplos de lenguaje y ejecutables NXE ya funcionan, pero eso no convierte el kernel existente en un kernel NEXTRY.

El siguiente trabajo de lenguaje debe especificar e implementar las capacidades de sistemas necesarias antes de migrar componentes. La ruta tecnica de `AUTOALOJAMIENTO.md` sirve de base, bajo el nombre y requisito principal establecidos aqui.

Las extensiones `.nxf` y `.nxj` existentes son formatos fuente de los prototipos. No se han renombrado automaticamente ni se ha decidido una nueva extension definitiva para NEXTRY. `.nxe` sigue siendo el formato ejecutable propio de NXK.

## Cesion de turno en aplicaciones

El prototipo incorpora la orden `ceder`, sin argumentos. Una aplicacion NXE2
cargada en la ruta cooperativa puede entregar su turno al kernel y continuar
despues con su contexto conservado. El runtime CPL3 utiliza INT81 servicio9;
las rutas sin ese soporte y NXE1 fallan explicitamente. No implementa objetos,
hilos Java. Ceder es voluntario aunque la ruta de procesos tambien disponga
de interrupcion por PIT. Ver `REGLAS.md` y `ejemplos/proceso.nxf`.

## Espera de procesos

`dormir expresion` espera un numero entero de ticks PIT y continua con el
contexto conservado. El servicio CPL3 es INT81 numero10 (EBX=ticks), con retorno0
al reanudar. El kernel limita el intervalo a 0..0x7fffffff; dormir 0 equivale a
ceder. Las rutas sin soporte y NXE1 terminan con fallo explicito. Consultar
`REGLAS.md` y `ejemplos/proceso_dormir.nxf` para sintaxis y limites.
