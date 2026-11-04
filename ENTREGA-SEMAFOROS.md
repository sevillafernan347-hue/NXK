# NXK 0.0.3: sincronizacion y crecimiento del arranque

Base bfee677. Integracion principal incorpora ocho semaforos contadores kernel
con IDs de generacion, adquirir con plazo, publicar, consultar y destruir.
Se agrega ESPERANDO_SEMAFORO al final de los estados. No cambia NXE ni INT81.
Puente C y cambio de contexto NEXTRY ASM. Se amplia arranque propio a tres
lecturas BIOS de 32/32/56 KiB, con imagen hasta 120 KiB y reserva total 128 KiB.
Los primeros 8 MiB ya estan excluidos del asignador: no cambia el heap ni
las direcciones de procesos o tablas. Se verificaran limites y lectura final.

## Sincronizacion

Ocho semaforos contadores con capacidad maxima configurable, disponibles y
esperadores consultables por ID. Los IDs usan generacion y no reaparecen al
reciclar un slot: si se agotan generaciones, el slot se retira. Pertenecen a su
propio espacio de nombres; no intercambiar IDs de tarea con IDs de semaforo.
El registro usa 160 bytes estaticos y las esperas no asignan heap.

Crear exige maximo positivo e inicial <= maximo; una tabla llena no cambia el
ID de salida. Adquirir solo desde tarea con IF habilitada consume un permiso
disponible o duerme hasta publicar/plazo. Cero ticks consulta; se rechazan
intervalos mayores de 2^31-1. El plazo original no se renueva al competir por
un permiso. Un permiso disponible al reanudar tiene prioridad sobre vencer.

Publicar no requiere propietario y puede hacerse desde kernel fuera de tarea.
Devuelve error si ya esta al maximo, sin desbordar UINT32_MAX. Despierta los
esperadores para competir en las rondas normales; no hay cola FIFO estricta
ni reserva de permiso para quien desperto primero. El modelo es una CPU y
fuera de IRQ. No es un mutex con propietario ni un mecanismo SMP.

Destruir devuelve -3 si quedan referencias de espera, incluso si la tarea ya
fue despertada pero no reanudo. Cancelar un esperador libera esa referencia;
no ejecuta su continuacion. Cancelar una tarea que ya adquirio permisos no los
devuelve automaticamente. El llamador administra los permisos y la destruccion;
no se mantienen listas de propietarios. El detector de ciclos envio/finalizacion
no deduce propietarios de semaforos; los plazos siguen siendo necesarios.

## Planificador y diagnosticos

Nuevo estado ESPERANDO_SEMAFORO, cancelable y con plazo PIT. El resumen cuenta
estas tareas entre pendientes y muestra su cantidad; listar publica el ID de
semaforo en espera a. Estadisticas agrega esperas_semaforo, contador saturado
reiniciado por generacion. Los registros de metricas pasan de 320 a 352 bytes.
No cambia NXE, INT 0x81 ni la sintaxis NEXTRY. El puente sigue en C; el cambio
de pila sigue en kernel/tareas32.nxa. Interfaz nueva: lib/semaforos.h.

## Arranque y memoria

Lecturas de 64/64/112 sectores desde LBA 1/65/129 hacia direcciones fisicas
0x10000/0x18000/0x20000. Ninguna cruza frontera fisica de 64 KiB ni supera
127 sectores. Se preserva DX/unidad BIOS y se recargan DS/ES y paquete entre
lecturas. La firma y tamano del arranque siguen siendo 512 bytes propios NXA.

Imagen maxima 120 KiB, imagen+BSS menor de 128 KiB. Reserva baja hasta 0x30000;
los primeros 8 MiB ya estaban excluidos del asignador fisico. No se cambia el
heap 1 MiB, las tablas de paginas ni la base virtual de apps. x64 conserva su
arranque de 32 KiB; esta sincronizacion nueva devuelve no disponible (-64).

## Pruebas y uso

```
version
semaforos
tareas resumen
tareas estadisticas
```

Ocho tareas comparten un permiso y ejecutan 32 operaciones, cediendo dentro
de la seccion protegida para comprobar exclusion. Se repite sin crecer heap
ni guardas. Las autopruebas tambien cubren tabla de ocho llena, validacion de
parametros, dos permisos, limite UINT32_MAX, IDs viejos, salida intacta con
consulta rechazada, espera/plazo, despertar y cancelacion. Destruir se rechaza
antes y despues del despertar hasta que el esperador reanude.

La consola intenta diez veces iniciar la demo sin slots de tarea libres:
rechaza sin alterar el lote original y luego ejecuta nuevas demos con exito,
para detectar fugas de objetos. La API y los cuerpos de prueba se separan en
lib/semaforos.h y kernel/semaforos.c; la espera usa el planificador.

pruebas_arranque_bloques.py ejecuta una carga NEXTRY ASM de 120 KiB y comprueba
cinco marcas en ambos lados de las transferencias y al final. Un caso valido
y cinco corrupciones controladas verifican que se lee toda la carga. Se usan
imagenes virtuales snapshot; las bases quedan intactas. No certifica ASUS.

Verificacion final ejecutada: matriz x86 32/64/128/1024 MiB; regresion x64
64 MiB; siete pruebas de ensamblador; seis escenarios de carga completa;
consola del planificador/general; dieciseis escenarios NXE2; diez escenarios
de excepciones, doble fallo y privilegios. Imagen de pruebas x86 61452 bytes
(supera el primer bloque), normal 45068 bytes. Boot final 512 bytes con firma
55aa. Todos los casos completados; pruebas virtuales con discos snapshot.
