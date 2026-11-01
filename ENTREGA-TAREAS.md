# Tareas cooperativas iniciales

Base: 23d23d7. Cambio de pila y registros preservados por ABI en
kernel/tareas32.nxa; organizacion y demostracion en kernel/tareas.c (puente C).
Comando tareas: dos tareas alternan cuatro veces cada una y comprueban sus
variables locales al reanudarse. Cada tarea usa una pila privada de 4096 bytes
del heap. Las reservas se reutilizan al repetir la demostracion.

Validacion: test.ps1 -Architecture x86 -MemoryMatrix superado con 32, 64, 128 y
1024 MiB: TAREAS COOPERATIVAS OK y BOOT PASS en cada registro. Regresion
x64 con 64 MiB superada; tareas nuevas solo disponibles en x86.
Imagenes normales reconstruidas.

Limites: demostracion de cambio de contexto, sin API publica de procesos,
sin preempcion ni aislamiento, sin proteccion de desbordamiento de pila.
No conserva estado flotante/SIMD; compilacion del kernel sin esas extensiones.
No cambia EFLAGS ni segmentos; las tareas comparten privilegios y estado de
interrupciones y deben mantenerlos. No llamar desde una IRQ. El PIT cuenta
el tiempo, pero aun no decide que tarea ejecutar. No ejecutar codigo externo
como tarea. Siguiente etapa: estados de tarea y finalizacion controlada.

Refuerzo de pilas: sello al inicio y validacion del puntero guardado antes y
despues de cada cambio. Pruebas negativas con sello alterado y puntero fuera
del margen valido, sin activar la pila corrupta. Repeticion de demostracion
sin consumo adicional de heap. Matriz x86 32/64/128/1024 MiB superada.
Esta deteccion es posterior a posibles escrituras: no impide desbordamientos
ni ofrece aislamiento. Para eso siguen pendientes memoria virtual y guardas.
No se incorporo codigo Linux ni se midio superioridad respecto de Linux.
