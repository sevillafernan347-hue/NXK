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

## Esperas de tareas mediante PIT

Estado DORMIDA con plazo por tarea. El organizador revisa plazos, ejecuta tareas
listas y usa HLT cuando todas esperan. Comparacion modular de ticks probada
antes/despues de vuelta de uint32_t y en igualdad. Intervalos admitidos menores
de 2^31 ticks; la demostracion usa 1 y 2 ticks. Cada trabajo retorna a una
entrada que marca TERMINADA y cede para siempre; no retorna fuera de su pila.
Se rechaza iniciar la demo si IF esta deshabilitada. El organizador tiene cota
de 100 ticks entre retornos; una tarea que nunca ceda aun bloquea el kernel.

Pruebas: cada tarea verifica variables locales y que su espera no termine antes
de tiempo. Se conserva prueba de reutilizacion y limites de pilas. Matriz x86
32/64/128/1024 MiB y regresion x64 64 MiB superadas. No hay preempcion, procesos
ni API general de creacion de tareas todavia. Organizadores C, cambio NXA.
