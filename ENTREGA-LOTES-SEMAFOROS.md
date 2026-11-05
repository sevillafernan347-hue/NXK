# Semaforos con reservas por lotes

Base: v0.0.4, commit 977c3b9. Integracion principal, v0.0.5.
Puente C en kernel/planificador.c sobre el cambio de contexto NEXTRY ASM.

## Problema y comportamiento

Una tarea que necesita tres unidades de un recurso no debe adquirir dos y
quedarse esperando la tercera: otras tareas podrian necesitar esas dos para
avanzar. Ahora la reserva es indivisible: toma todas las unidades solicitadas
o espera conservando intacto el contador. No hay asignaciones de heap nuevas.

## API kernel

Declaraciones en lib/semaforos.h:

```c
int nxk_semaforo_adquirir_n(uint32_t id, uint32_t cantidad, uint32_t ticks);
int nxk_semaforo_publicar_n(uint32_t id, uint32_t cantidad);
```

Cantidad debe estar entre uno y el maximo del objeto. Cero y cantidades mayores
se rechazan con -1 sin modificar permisos. Un ID inexistente/antiguo devuelve -1.

Adquirir exige una tarea actual con interrupciones habilitadas. Devuelve 0 si
toma el lote completo, 1 si no hay suficientes permisos en consulta sin espera
o vence el plazo, -1 ante contrato invalido. El plazo maximo es 0x7fffffff ticks;
cada despertar conserva el plazo original. Como en adquirir un permiso, los
permisos disponibles al reanudar tienen prioridad sobre el vencimiento.

Publicar admite el contexto kernel exterior a las tareas. Devuelve 0 al sumar
todo el lote o -2 si excederia el maximo, sin sumar parcialmente. La comprobacion
usa maximo menos disponibles antes de sumar, evitando overflow uint32.
Publicar despierta a los esperadores, que vuelven a competir y comprobar su
cantidad completa; despertar no concede ni reserva permisos.

Las funciones anteriores adquirir/publicar delegan en lotes de una unidad:
se conservan sus contratos. La espera usa el estado, las referencias y metricas
de semaforo existentes. No cambia la ABI de aplicaciones NXE ni las palabras
NEXTRY. La cantidad pendiente permanece en la pila de la tarea.

## Cancelacion y limites

Cancelar una tarea bloqueada elimina su referencia sin consumir unidades.
Destruir sigue rechazado mientras existan referencias, incluso entre despertar
y reanudacion. Semaforos no tienen propietario: cancelar despues de adquirir
no devuelve automaticamente los permisos. Para propiedad y abandono usar mutex.
Una CPU, cooperativo, fuera de IRQ; no ofrece FIFO, prioridad ni ausencia de
inanicion para lotes grandes que compiten con adquisiciones pequenas.

## Pruebas

kernel/semaforos.c incluye casos de cantidad cero/excesiva, consulta insuficiente,
reserva completa, publicacion completa y overflow con UINT32_MAX, ID destruido,
espera con despertares insuficientes, plazo sin consumo parcial y cancelacion.
Consulta contadores y referencias en cada etapa y comprueba heap/guardas estables.
La matriz x86 ejecuta estas pruebas dentro del kernel con NXK_TEST.
