# NXK 0.0.4: mutex con propietario y recuperacion explicita

Base 55f281f. Integracion principal aprueba ocho mutex kernel con IDs de
generacion, plazo, propietario, recuperacion y deteccion de ciclos combinada.
Si un propietario termina/cancela, el recurso se marca abandonado y el siguiente
adquirente recibe 2: debe reparar datos y declarar consistente antes de liberar.
Sin cambio a NXE ni INT81. Puente C, una CPU y cambio de contexto NEXTRY ASM.

## Propiedad y estados

Registro estatico de ocho mutex con IDs de generacion y espacio de nombres
propio. Solo la tarea propietaria puede liberar o declarar consistente. Se
rechazan recursion, IDs viejos, destruccion con propietario y destruccion con
referencias de espera aun no consumidas. No se asigna heap por adquirir o crear.

| Situacion | Resultado de adquirir |
| --- | --- |
| Libre y sano | 0, toma propiedad |
| Propietario anterior termino/cancelo | 2, toma propiedad y exige reparar |
| Ocupado, cero ticks o plazo vencido | 1, no toma propiedad |
| Ya pertenece a la tarea actual | -2, no es recursivo |
| Nueva espera formaria ciclo | -4 |
| Liberado tras abandono sin declarar consistente | -5, irrecuperable |

La respuesta 2 significa adquirido, no fallo: el llamador repara los datos
protegidos y llama a nxk_mutex_consistente antes de liberar. El kernel no
sabe reparar los datos de otro componente. Liberar sin declararlo consistente
marca el objeto irrecuperable y despierta al resto para devolver -5. No se
puede borrar ese estado declarando consistente desde otra tarea. Destruir un
objeto sin propietario/referencias permite recrearlo con otro ID y estado sano.

Si el nuevo propietario vuelve a morir mientras repara, el siguiente observa
abandono de nuevo; no se considera automaticamente consistente. Retorno normal,
finalizar y cancelacion usan el mismo cierre del planificador. Ese cierre
abandona todos los mutex que pertenecian a la tarea, no solo el ultimo.
No recupera fallos fatales del kernel ni deshace efectos sobre dispositivos.

## Esperas y ciclos

Nuevo estado ESPERANDO_MUTEX, plazo original fijo menor de 2^31 ticks y
reintento sin alargar el plazo. Mutex libre/abandono/irrecuperable al reanudar
prima sobre el vencimiento. Cancelar un esperador elimina su referencia sin
ejecutar su continuacion. No hay cola FIFO estricta ni herencia de prioridades.

El detector sigue al propietario cuando encuentra una espera de mutex. Puede
rechazar cadenas que mezclen mutex, envio con buzon lleno y esperar_fin. Los
semaforos siguen sin propietario conocido; no se infieren sus dependencias.
Las tareas listas tras evento o timeout no prolongan cadenas bloqueadas.

El resumen del planificador y listar muestran las esperas de mutex. Estadisticas
agrega esperas_mutex saturado; registros de metricas totales pasan a 384 bytes.
La tabla mutex usa 192 bytes estaticos, sin reservas nuevas de heap o pilas.

## Verlo en NXK

```
version
mutex
mutex recuperar
tareas resumen
tareas estadisticas
```

mutex ejecuta ocho tareas y 32 operaciones sobre un valor compartido, con
cesiones dentro de la seccion protegida. mutex recuperar deja datos incompletos
al terminar el propietario; otra tarea detecta el abandono, repara el valor,
lo declara consistente y vuelve a adquirir normalmente el mismo mutex.

La API esta en lib/mutex.h. El registro y cierre por propietario se integran
en kernel/planificador.c; demos y pruebas estan en kernel/mutex.c. La
construccion enlaza el modulo nuevo tanto en x86 como en la regresion x64,
que devuelve no disponible (-64). No se agregan llamadas CPL3 ni palabras
NEXTRY; conserva puentes C y cambio de contexto propio NEXTRY ASM.

## Pruebas

Ocho slots llenos y salida intacta al rechazar el noveno; generacion nueva,
consulta vieja sin alterar salida, propiedad exclusiva, recursion y liberacion
ajena rechazadas. Plazo de espera, cancelacion de esperador y destruccion
protegida incluso despues de despertar. Cancelar propietario con dos mutex
abandona ambos; el siguiente los recupera explicitamente. Se comprueba retorno
normal abandonando, reparacion valida y liberacion sin reparar que impide
adquirir de nuevo. Ciclo entre dos mutex y ciclo mutex/finalizacion rechazados.

La consola repite demos de exclusion y recuperacion tres veces. Tambien intenta
ambas diez veces sin slots de tarea: rechaza sin ocupar recursos y despues
permite ejecutar nuevos lotes. Heap y guardas se conservan tras todas las demos.

Limites: una CPU, ring0, fuera de IRQ y planificador cooperativo. Un callback
que nunca ceda puede bloquear el kernel; esta ampliacion no introduce procesos
usuario simultaneos, SMP ni preempcion. Los datos se reparan por el llamador.

Verificacion ejecutada: matriz QEMU x86 32/64/128/1024 MiB; regresion x64
64 MiB; seis casos del cargador de 120 KiB; consola general/planificador;
dieciseis escenarios NXE2; diez escenarios de excepciones, doble fallo y
privilegios. Las demos normales y de recuperacion se repiten sin crecer heap
ni guardas. El marcador MUTEX ROBUSTOS OK es obligatorio en test.ps1 x86.
