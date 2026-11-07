# NXK v0.0.15: procesos en espera por tiempo

Base v0.0.14, commit48859b8. Espera temporizada CPL3 sobre los espacios privados,
registros conservados, preempcion y planificador de rondas existentes.
Puentes C en el motor y PCB; entrada/reanudacion NEXTRY ASM sin cambios.

## Usar desde NEXTRY

```text
funcion inicio
    entero valor = 23
    dormir 20
    afirmar valor == 23
    numero valor + 19
fin
```

Los ticks PIT son aproximadamente100 por segundo:20 son unos200ms, no20ms.
`dormir expresion` exige tipo entero;0 equivale a `ceder`. El maximo valido es
0x7fffffff. Un intervalo mayor se rechaza y el runtime NEXTRY termina con error.
El ejemplo completo proceso_dormir.nxf tambien comprueba dormir0 y variables.

Compilar/empaquetar NXE2 como las otras apps y ejecutar por `proceso cargar`,
`proceso paso` o `proceso ejecutar`. `proceso dormir` es una demostracion que
espera20 ticks con HLT mientras verifica el avance de otro programa; su vuelta
normal comprueba registros, pila y paginas devueltas.

## Estados y planificacion

DORMIDO5 se agrega a los estados anteriores sin renumerarlos. Dormir conserva
el contexto posterior a INT81, retorna EAX0 al continuar y mantiene EBX con el
intervalo pedido. Los datos de pila y los demas registros siguen siendo privados.
Solo un intervalo positivo incrementa `dormidas`; cero incrementa `cesiones`.

El plazo absoluto se calcula dentro de INT81 y se publica aparte del marco.
Asi no se reutiliza EBX como dato interno ni se retrasa el inicio de la espera
hasta que vuelva el planificador. `despertar` se limpia al ejecutar, despertar,
fallar o liberar. Su valor0 tambien puede ser un plazo valido tras envolver el
reloj: siempre se decide por el estado DORMIDO, nunca por despertar!=0.

Al consultar/listar o recorrer procesos se revisan IDs y plazos. Un vencido
vuelve a LISTO y continua cuando recibe otro turno. Antes del plazo no consume
turnos y `proceso paso ID` se rechaza por estado. No hay un planificador de fondo
que lance aplicaciones mientras la consola espera un comando.

`nxk_procesos_ejecutar` retorna:

| Codigo | Significado |
| --- | --- |
| 0 | Sin procesos pendientes |
| 1 | Presupuesto de turnos agotado con procesos listos |
| 2 | Quedan dormidos, sin listos; retorna inmediatamente |
| Negativo | Contrato, contexto o motor invalido |

En el caso2 la consola indica que hay procesos esperando y permite dar otro
comando o volver a ejecutar despues. No se espera activamente dentro del
planificador. Otro proceso LISTO puede terminar aunque uno este dormido.
Liberar un dormido cancela su espera y borra el PCB; un slot reciclado nunca
recibe el plazo de un ID anterior.

## Contrato de servicio

Extension opcional INT81/EAX10, EBX=intervalo0..0x7fffffff. Solo disponible en
la ruta de procesos reanudables. La ABI/cabecera NXE2 sigue en1; servicios1..9
conservan su comportamiento. Sin soporte, el runtime falla explicitamente;
NXE1 no simula una espera. En codigo kernel el puente NEXTRY delega a
nxk_tarea_dormir y exige una tarea actual valida.

El motor interno ahora retorna un quinto argumento de plazo junto al motivo3
DORMIR. Sus llamadores y stub x64 se actualizan juntos; no es una nueva
estructura accesible por el ejecutable. Se reinicia el plazo en cada turno y
solo se publica con motivo DORMIR. Los otros motivos publican plazo0.

La comparacion modular usa ahora-plazo < 0x80000000, con plazos limitados a
media vuelta del contador32bits. La consulta debe ocurrir antes de otra media
vuelta despues del vencimiento (unos248 dias a100Hz). No se incorporo un reloj
monotonico64bits; esta es una limitacion explicita del temporizador inicial.

## Pruebas y limites

Matriz QEMU x86 32/64/128/1024MiB y regresion x64 64MiB. El arranque prueba otro
proceso que termina durante una espera, ausencia de turnos extra al dormir,
EAX0, EBX/ESI/ESP/sello conservados al continuar, tres repeticiones sin fuga,
cero, maximo y rechazos80000000/ffffffff. Se comprueba aritmetica de igualdad,
wrap y deadline0, y que IDs viejos no se reactivan.

Prueba externa `lenguaje/pruebas_dormir.py`: NEXTRY reanudado varias veces,
solo-dormidos devolviendo la consola, otro programa terminando, cero, maximo,
rechazos, liberacion/reciclaje y espera vieja vencida sin alterar el nuevo ID.
Se comparan heap/paginas/guardas, historial de cargas rechazadas y discos
intactos. Despues siguen funcionando las tareas, NEXTRY y preempcion anteriores.
Compilador:24pruebas, incluida sintaxis y tipo de dormir.

No son esperas de dispositivos ni IPC: eso sigue pendiente, junto a permisos,
mas recursos por proceso y SMP. El kernel y syscalls siguen cooperativos;
solo CPL3 gestionado tiene preempcion. La ASUS fisica no esta certificada.
