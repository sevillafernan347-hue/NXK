# Objetivo: NXK completo escrito en NEXTRY

La definicion principal y el nombre definitivo estan en `NEXTRY.md`: todo el sistema operativo y kernel deben basarse en NEXTRY.

Requisito del usuario: el sistema debe estar programado en el lenguaje que estamos creando, incluidas sus partes de bajo nivel. Se conserva como objetivo de arquitectura; todavia no esta cumplido.

## Lenguaje principal

NEXTRY sera la base comun del sistema y las herramientas. NXK Espanol (`.nxf`) y NXJ (`.nxj`) son prototipos previos aprovechables para su construccion. Las capacidades de Java forman parte del objetivo de comodidad del lenguaje. Los ejecutables nativos seguiran usando `.nxe`.

Esto evita mantener dos implementaciones independientes del mismo runtime. La compatibilidad de programas Windows sera una capa del sistema, no su fundamento.

## Dependencias actuales que debemos sustituir

| Componente actual | Implementacion actual | Sustitucion necesaria |
|---|---|---|
| Arranque y entrada al nucleo | Ensamblador `.S` | Primitivas de procesador, secciones y entrada implementadas por el compilador propio |
| Nucleo, memoria y cargadores | C | Tipos de sistemas, estructuras, punteros, disposicion binaria y llamadas con ABI definida en NXF |
| Compiladores de NXF y NXJ | Python | Compilador propio escrito en NXF/NXJ |
| Generacion de instrucciones | C generado y Clang/LLD | Backend y enlazador propios, o una dependencia de backend explicitamente documentada durante la transicion |
| Construccion | Scripts PowerShell | Herramientas propias cuando existan archivos, procesos y entorno suficientes |

El procesador ejecuta instrucciones maquina: el compilador debe generarlas desde nuestras construcciones de bajo nivel. Eso permite que la fuente sea propia sin ocultar ensamblador o C dentro de archivos renombrados.

## Secuencia de migracion

1. Especificar tipos de ancho fijo, parametros, retornos, estructuras y operaciones de memoria.
2. Implementar esas capacidades y comprobar su comportamiento en ambas arquitecturas.
3. Migrar primero componentes pequenos, como cadenas y el asignador de laboratorio, con pruebas comparativas.
4. Migrar consola, cargadores y componentes del nucleo.
5. Incorporar intrinsecos de puertos, registros, interrupciones y arranque al compilador; despues sustituir las fuentes ensamblador.
6. Escribir el compilador en el propio lenguaje. Usar el compilador inicial para construirlo y comprobar que el nuevo compilador puede recompilarse: autoalojamiento.
7. Sustituir el backend y las herramientas externas segun la cobertura alcanzada.

Hasta que una sustitucion funcione y pase sus pruebas, se conserva la implementacion inicial para mantener NXK arrancable. El compilador actual sigue generando C; aun no genera directamente instrucciones maquina y no es autoalojado.

## Criterio de finalizacion

Inventariar las fuentes necesarias para construir el sistema, comprobar que estan escritas en el lenguaje propio y documentar cualquier herramienta externa que siga siendo necesaria. Autoalojamiento del compilador y eliminacion de dependencias externas son hitos distintos: ambos deben demostrarse, no inferirse por las extensiones de los archivos.
