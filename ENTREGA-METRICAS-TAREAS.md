# Metricas del planificador

Base e399211. Integracion principal amplia API kernel con consulta de metricas
por ID. Contadores uint32 saturados, reiniciados al crear otra generacion y
conservados mientras la tarea terminal no se libere. Sin cambios NXE/CPL3.
La implementacion sigue como puente C y usa el cambio de contexto NEXTRY ASM.

La imagen de pruebas necesita leer datos mas alla del antiguo limite. BIOS x86
pasa a 120 sectores (60 KiB), por debajo de 127 sectores y del segmento de
64 KiB. Linker/build verifican 60 KiB de archivo y mantienen el limite total
de imagen+BSS en 64 KiB. x64 conserva 32 KiB. No se alteran las bases ABI.

## Consulta

`nxk_tarea_metricas(id,&salida)` devuelve una copia sin cambiar ni reiniciar
los contadores. Un ID nulo/liberado/viejo o salida nula se rechaza (-1) sin
modificar la salida. x64 devuelve -64. Se mantienen los datos de tareas
terminadas/canceladas hasta liberar y se limpian al crear otra generacion.

| Campo | Evento contado |
| --- | --- |
| activaciones | Entradas al contexto, incluida la primera y cada reanudacion |
| cesiones | Llamadas validas a ceder, incluido dormir(0) |
| dormidas | Esperas temporales positivas aceptadas |
| esperas_mensaje | Bloqueos reales por buzon vacio |
| esperas_envio | Bloqueos reales por buzon lleno, incluido volver a esperar |
| esperas_fin | Bloqueos reales esperando resultado |
| enviados | Mensajes encolados por esta tarea |
| recibidos | Mensajes retirados por esta tarea |
| plazos | Operaciones con plazo positivo que retornan vencimiento |
| ciclos | Esperas rechazadas por dependencia circular |

Consultas con cero ticks, errores de contrato y mensajes enviados desde el
kernel fuera de tarea no inflan estos contadores. Un mensaje/hueco/resultado
que este disponible al reanudar sigue teniendo prioridad; solo se contabiliza
un timeout si la operacion realmente lo devuelve. No mide porcentaje de CPU,
tiempo de ejecucion ni latencia. No hay reinicio manual que destruya evidencias.

Se usan 320 bytes estaticos para ocho registros; no hay asignaciones de heap
por consultar/contabilizar. Todos los contadores, incluidos los turnos antiguos,
saturan en UINT32_MAX. Un contador saturado deja de distinguir eventos nuevos.

## Verlo en NXK

```
tareas preparar
tareas estadisticas
tareas ejecutar
tareas estadisticas
tareas limpiar
```

Cada tarea muestra turnos, cesiones, dormir, mensajes, esperas, plazos y ciclos.
Las ocho tareas listas empiezan en cero. Cada trabajo normal del lote termina
con cinco activaciones y cuatro esperas temporales. Una tarea cancelada antes
de su primera activacion conserva todos los contadores en cero.

## Verificacion

Autoprueba con una tarea: una cesion, dormir positivo, mensaje propio y
recepcion inmediata, timeout de mensaje, buzon lleno y ciclo rechazado.
Se comprueban cuatro activaciones, cinco envios/recepciones, espera y plazo
exactos. Se verifica salida intacta con ID liberado, rechazo de salida nula,
reset al reutilizar un slot y saturacion sin vuelta a cero.

La consola comprueba contadores iniciales, resultados exactos de siete tareas
y una cancelada, consultas repetidas sin alterar datos, y limpieza en otra
generacion. Conserva las pruebas de flujo, resultados y bloqueos antes de
comprobar memoria estable y ejecutar NEXTRY protegido.

La API sigue siendo kernel ring0 de una CPU y fuera de IRQ. El mecanismo
cooperativo y los servicios de aplicaciones conservan sus contratos.

Verificacion ejecutada: matriz QEMU x86 32/64/128/1024 MiB, regresion x64
64 MiB, consola del planificador/general y dieciseis escenarios NXE2. El
marcador METRICAS TAREAS OK es obligatorio para aprobar test.ps1 en x86.
