# Espera de mensajes con plazo

Base 0b794e3, integracion principal. Se amplia la API kernel sin cambiar NXE ni
INT 0x81: nxk_tarea_esperar_mensaje(mensaje,ticks), retorna 0 con mensaje,
1 por plazo o consulta vacia con cero ticks, negativo por contrato invalido.
Estado nuevo ESPERANDO al final de la enumeracion, conserva valores anteriores.
Puente C para x86 cooperativo, una CPU, fuera de IRQ. Sin soporte CPL3 nuevo.

La imagen de pruebas supera el antiguo limite: el cargador x86 pasa a 112
sectores (56 KiB), paquete BIOS por debajo de 127 sectores y del segmento de
64 KiB. Linker y build comprueban 56 KiB; BSS mas imagen siguen limitados a
64 KiB en RAM. Base 0x10000 y todas las ABI existentes se mantienen. La
variante x64 conserva su cargador y limite de 32 KiB.

## Comportamiento

La tarea se marca ESPERANDO y devuelve el control al organizador. No consulta
continuamente el buzon ni ocupa turnos mientras no tenga mensaje/plazo vencido.
Enviar a una tarea esperando la deja LISTA; el planificador circular decide su
siguiente turno. Dormir conserva su espera temporal, incluso si recibe mensajes.
Cancelar admite ESPERANDO. IDs y estados anteriores mantienen sus valores.

La espera recibe primero cualquier mensaje disponible. Cero ticks consulta sin
bloquear; un plazo de 1 a 2^31-1 se compara con aritmetica modular. Al reanudar,
un mensaje disponible tiene prioridad sobre el vencimiento: no es una garantia
de entrega antes de una hora exacta. Un timeout no altera el mensaje de salida.
No se modifican los codigos de recibir/enviar no bloqueantes. Solo se acepta
esperar desde la tarea actual y con interrupciones habilitadas. No llamar en IRQ.

`tareas mensajes` ejecuta un emisor y receptor, intercambia 16 mensajes y verifica
origen, orden y despertar. Los argumentos viven hasta que ambas tareas terminan;
el lote se libera al retornar y puede repetirse sin crecer heap ni guardas.

## Pruebas ejecutadas

- QEMU x86 32/64/128/1024 MiB con marcador obligatorio ESPERA MENSAJES OK.
- Mensaje despierta ESPERANDO a LISTA, 16 valores FIFO con origen correcto,
  demo repetida, vacio con cero ticks y recepcion inmediata de mensaje previo.
- Timeout de tres ticks como minimo, salida intacta, puntero nulo, intervalo
  invalido y llamada fuera de tarea rechazados.
- Espera conserva contexto tras agotar presupuesto del organizador; envio
  desde kernel despierta la tarea y reanudacion entrega origen 0/valor esperado.
- Cancelar mientras espera no ejecuta la continuacion; despues rechaza envios.
- Consola interactiva: dos demos nuevas, memoria estable y NEXTRY posterior.
- Consola general y regresion x64 64 MiB (API nueva no disponible, -64).
- Dieciseis escenarios NXE2, incluyendo recargas despues de ocho tareas.

El mecanismo sigue siendo cooperativo, ring0 y una CPU. No introduce hilos de
aplicaciones, preempcion ni sintaxis NEXTRY nueva; la organizacion es puente C.
