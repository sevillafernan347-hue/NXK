# Planificador cooperativo reutilizable

Base: 274473c. Integracion principal.
Propuesta aceptada por la integracion: interfaz kernel en lib/tareas.h,
ocho tareas ring0, identificadores con generacion, estados y ejecucion acotada
por ticks entre cesiones. No modifica NXE, INT 0x81 ni el cambio NEXTRY ASM.
La organizacion y las pruebas se implementan como puente C. Las pilas tienen
pagina de guarda y se reutilizan; no se expone todavia a aplicaciones CPL3.

## Implementacion

- kernel/planificador.c: registro de ocho tareas, rondas, esperas PIT, IDs,
  estados, presupuestos y buzones. lib/tareas.h publica el contrato.
- kernel/tareas.c: trabajos con argumento y pruebas del ciclo de vida.
- kernel/tareas32.nxa: cambio de contexto existente, conservado y verificado
  byte a byte durante construccion. Guarda EBX, ESI, EDI, EBP y ESP.
- kernel/main.c: consola para preparar/listar/ejecutar/cancelar/limpiar.

Cada ID combina generacion de 24 bits con slot de ocho bits. Liberar invalida
el ID; recrear aumenta la generacion. Un slot que agote generaciones se retira
en lugar de reutilizar un ID viejo. No se liberan tareas que sigan ejecutando.
Cancelar solo admite lista o dormida, y no ejecuta finalizadores de recursos.
El llamador mantiene vivos los argumentos hasta terminar o cancelar/liberar.

Cada slot reserva 12 KiB de heap una sola vez para alinear una pagina ausente
de guarda y una pila util de 4 KiB. Los ocho slots reservan 96 KiB como maximo;
no se devuelve ese espacio al heap monotono. Crear de nuevo limpia la pila y
buzon, sin tocar la guarda. Repetir lotes no consume mas heap ni mas guardas.
Un sello y el rango de ESP se verifican antes y despues de cada despacho.
Rechazar una pila guardada corrupta ocurre antes de ejecutar su callback.
Una pagina de guarda contiene la escritura, pero un fallo de pila en ring0
sigue siendo fatal; el comprobador no promete recuperar un kernel dañado.

Estados: libre, lista, ejecutando, dormida, terminada y cancelada. Las terminales
se conservan para diagnostico hasta liberar. El cursor circular evita favorecer
siempre el slot cero. Dormir acepta 0 como ceder e intervalos menores de 2^31;
se comparan deadlines con aritmetica modular, incluyendo la vuelta de uint32_t.

Ejecutar devuelve 0 si no quedan tareas pendientes y 1 si vence el presupuesto.
El segundo resultado conserva las tareas para reanudar. Se rechazan presupuestos
cero o mayores de 2^31-1, invocaciones anidadas e interrupciones deshabilitadas.
El limite se comprueba entre callbacks/cesiones: no interrumpe un bucle ring0.

## Buzones

Cada tarea tiene cuatro mensajes FIFO con origen y valor uint32_t. Enviar no
bloquea: -2 significa lleno; recibir devuelve -2 si esta vacio y conserva la
salida del llamador. Enviar desde kernel fuera de tarea identifica origen 0;
desde una tarea registra su ID completo. Destinos terminados/cancelados se
rechazan. Liberar y recrear limpian mensajes, sin entregarlos a otra generacion.
No hay punteros de payload ni despertar anticipado de dormir. La ampliacion
[espera de mensajes](ENTREGA-ESPERA-MENSAJES.md) agrega espera con plazo y
despertar por envio conservando la interfaz no bloqueante original.
Las tareas pueden usar las variantes con espera de mensaje/espacio descritas
en ENTREGA-ESPERA-MENSAJES.md y ENTREGA-ENVIO-CON-PLAZO.md. Estas operaciones solo
son validas en la CPU kernel y fuera de IRQ, igual que el planificador.

## Ver y probar desde NXK

```
tareas preparar
tareas listar
tareas paso
tareas listar
tareas ejecutar
tareas listar
tareas limpiar
```

`tareas cancelar ID` cancela una lista/dormida por el numero que muestra listar.
El parser rechaza vacio, signos, caracteres extra y desbordamientos de uint32_t.
`tareas ocho` ejecuta y verifica un lote completo; `tareas` conserva la demo de dos.
Los argumentos del lote interactivo viven en memoria estatica entre comandos.

## Verificacion ejecutada

- Matriz QEMU x86 de 32, 64, 128 y 1024 MiB, con marcadores obligatorios nuevos.
- Ocho contextos y argumentos, 24 turnos con mismo orden circular, variables
  locales conservadas, esperas minimas y finalizacion retornando del callback.
- Registro lleno, parametros invalidos, IDs obsoletos y doble liberacion;
  crear desde otra tarea, rechazar ejecucion anidada y cancelar otra tarea.
- Presupuesto y reanudacion, cancelacion dormida, IF deshabilitada, limites de
  ESP/sello, rechazo real de contexto corrupto y comparacion con vuelta del PIT.
- FIFO de cuatro, quinto mensaje rechazado sin reemplazo, nueve vueltas de
  envio a uno mismo, vacio sin alterar salida y 32 mensajes productor/consumidor.
- Creaciones tras cancelar limpian buzones; terminales e IDs viejos no reciben.
- Lotes repetidos sin crecimiento de heap ni numero de guardas.
- Prueba interactiva de consola en lenguaje/pruebas_planificador.py: estados,
  pasos, cancelacion, generacion y limpieza, mas NEXTRY despues de las tareas.
- Regresion x64 64 MiB: API nueva retorna no disponible (-64).
- 22 pruebas del compilador y siete de NEXTRY ASM. Consola general superada.
- Dieciseis escenarios NXE2. El caso correcto ejecuta ocho tareas antes de
  cargar apps desde NXFS; las tres recargas siguen sin crecer el heap.

## Limites

Una CPU, tareas kernel cooperativas y callbacks confiables. Sin SMP, preempcion,
prioridades, contexto FPU/SIMD, procesos CPL3 simultaneos ni aislamiento por tarea.
No llamar desde IRQ; los callbacks no deben deshabilitar interrupciones. El
buzon no es una nueva llamada de sistema ni una instruccion NEXTRY. La futura
exposicion a apps necesitara permisos y validacion de buffers/identificadores.
El kernel conserva puentes C. Las pruebas virtuales no certifican la ASUS.
