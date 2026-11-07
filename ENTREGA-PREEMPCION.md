# NXK v0.0.14: turnos automaticos para procesos CPL3

Base v0.0.13, commit858834c. El temporizador ahora suspende procesos gestionados
sin exigir que llamen a `ceder`, conservando su contexto para reanudarlos.
Puente C sobre las entradas IRQ0 e IRET de NEXTRY ASM existentes.

## Uso

`proceso preempcion` demuestra un proceso ocupado que nunca cede, otro que
termina y la reanudacion del primero tras dos cortes del temporizador. El kernel
pone una bandera de parada en su memoria privada; el programa comprueba sus
registros, pila y banderas antes de terminar normalmente con42.

Para aplicaciones normales se conservan los comandos cargar/listar/paso/ejecutar/
liberar. `proceso paso ID` ejecuta hasta ceder, salir, fallar o agotar un turno.
`proceso ejecutar` recorre los procesos listos por rondas hasta32 turnos. Si un
bucle sigue listo, informa presupuesto agotado y retorna a consola; se puede
volver a ejecutar o liberar el ID. Un bucle ya no se considera fallo en esta
ruta. `proceso listar` diferencia cesiones voluntarias y preempciones.

El ejemplo NEXTRY `lenguaje/ejemplos/proceso_ocupado.nxf` contiene un bucle sin
ceder. El volumen generado por `lenguaje/pruebas_preempcion.py` queda en
build/x86/preempcion-pruebas/preempcion.img: incluye bucle.nxe, proceso.nxe,
fin.nxe, fallo.nxe y nulo.nxe. Conectarlo con run.ps1 -Volumen esa_ruta.

## Cambio en el motor

Tras10 ticks PIT desde el comienzo del turno (aproximadamente100ms a100Hz), una
IRQ0 procedente de CPL3 gestionado copia el marco completo de17 DWORD a su
contexto reanudable. NO sustituye EAX: no es una llamada al sistema. El EOI del
PIC ya se envia antes de devolver el control al planificador. IRET reanuda el
EIP, ESP, GPR, segmentos y banderas validadas que guardo esa interrupcion.

La API interna retorna un motivo:0 salida/fallo,1 cesion voluntaria,2 quantum.
Ambas suspensiones dejan el proceso LISTO; quantum incrementa preempciones y
ceder incrementa cesiones, con contadores saturados. Una preempcion no marca
fallo, conserva las paginas del proceso y permite ejecutar otros en la ronda.
Los registros EIP/ESP de la interrupcion quedan en el ultimo diagnostico.

Las rutas sincronicas anteriores sin contexto reanudable conservan su politica:
un bucle termina con vector32 y devuelve la consola. No cambia la cabecera
NXE2 ni los servicios. El bloque FPU/MMX/SSE y validacion de marcos de v0.0.13
siguen activos. Cada nuevo turno tiene un inicio de reloj propio.

## Evidencia

Pruebas internas: A gira sin ceder; B devuelve7 en la misma ronda. A es
interrumpido dos veces, su contador avanza y no pierde EAX/ECX/EDX/ESI/EDI/EBP,
ESP ni el valor apilado. Tras recibir bandera de parada, confirma DF preservado
y FS/GS nulos, limpia DF y devuelve42. Tres repeticiones mantienen heap, paginas,
guardas y CR3 kernel. Se restaura el mapa antes de limpiar tambien ante errores.

Matriz x86 QEMU32/64/128/1024MiB superada; regresion x64 64MiB superada.
La prueba externa carga un bucle NEXTRY sin ceder junto a un programa NEXTRY
cooperativo, una salida normal y UD2. Los otros completan/fallan como corresponde
mientras el bucle sigue LISTO. Otra ronda concede32 turnos adicionales al bucle;
la consola vuelve y se liberan todas sus paginas. Un bucle con DS/ES/FS/GS nulos
sobrevive a dos preempciones. Los discos permanecen intactos y la ruta legacy
sigue diagnosticando su bucle como fallo32. Procesos cooperativos, espacios
privados, consola, tareas kernel y22 escenarios NXE2 siguen pasando.

## Limites

La preempcion se aplica al codigo CPL3 de los procesos gestionados. El kernel,
las tareas kernel y los servicios de sistema no se interrumpen para cambiar de
proceso. El tiempo transcurrido en un servicio puede retrasar el corte hasta
volver a CPL3;10 ticks no son un limite de tiempo real estricto. Las rondas son
sincronicas con la consola y tienen presupuesto de turnos.

Continuan pendientes espera de E/S, mas recursos y permisos por proceso, SMP y
contexto FPU. No es preempcion general del kernel. El modo x64 solo conserva
regresion de arranque y no incorpora estos procesos. Las pruebas QEMU no
certifican el hardware ASUS.
