# Ocho sesiones de avance para NXK

La comparacion anterior de «menos del 1% de Windows» no se baso en una medicion.
Este plan mide entregas propias de NXK, no porcentajes de Windows. Son objetivos
de trabajo; una sesion puede requerir continuacion si aparecen fallos.

| Sesion | Objetivo | Evidencia para darlo por terminado |
| --- | --- | --- |
| 1 | Reforzar la memoria y preparar la deteccion de RAM | Limites del heap comprobados y mapa de RAM BIOS recibido por el kernel |
| 2 | Reservar y liberar paginas fisicas | Reutilizar paginas sin tocar zonas reservadas, con pruebas de agotamiento |
| 3 | Mejorar diagnostico de excepciones | Mostrar numero de excepcion y direccion del fallo en casos provocados |
| 4 | Incorporar temporizador e interrupciones | Contador de ticks que avance sin bloquear la consola |
| 5 | Introducir tareas del kernel | Dos tareas con pilas propias alternandose de forma comprobable |
| 6 | Crear archivos en RAM | Crear, listar, leer y borrar archivos con limites de capacidad |
| 7 | Ampliar NEXTRY para usar los servicios nuevos | Ejemplos comentados compilados y ejecutados en NXK |
| 8 | Integrar y documentar | Pruebas x86 con 32/64/128/1024 MiB, guia y lista exacta de limites |

## Condiciones del proyecto

Objetivo principal: x86 de 32 bits para equipos con 1 GB de RAM.
Usar NEXTRY/NEXTRY ASM para los componentes nuevos cuando sus capacidades
lo permitan; cualquier puente C debe identificarse y anotarse para migracion.
Mantener comentarios en espanol. Las pruebas virtuales no certifican la ASUS.
La biblioteca de 200 libros en espanol sigue como trabajo pendiente separado.

## Hito 1: implementado y probado en QEMU

Primer paso: informar memoria disponible del heap y comprobar que el asignador
rechace desbordamientos y agotamiento sin alterar el estado ni los datos previos.
La deteccion BIOS E820 ya esta implementada en NEXTRY ASM y recibida por el
kernel mediante un puente C. Pruebas x86 de RAM 32/64/128/1024 MiB superadas;
regresion x64 con 64 MiB superada, aunque esa variante no detecta RAM todavia.
El hardware ASUS sigue sin probarse.

## Hito 2: gestor inicial implementado y probado en QEMU

El gestor marca paginas completas de RAM, excluye cualquier pagina tocada por
una region reservada y evita duplicados entre rangos superpuestos. Reserva los
primeros 8 MiB y administra hasta 1 GiB. Verifica el espacio de sus metadatos antes
de escribirlo. Reserva, liberacion, reutilizacion, rechazo de doble liberacion y
agotamiento comprobados con 32/64/128/1024 MiB. Regresion x64 de arranque superada;
el gestor nuevo solo esta habilitado en x86.

Limites: puente C, sin concurrencia, sin memoria virtual, sin borrado automatico
de las paginas entregadas. El siguiente hito es diagnostico de excepciones.
