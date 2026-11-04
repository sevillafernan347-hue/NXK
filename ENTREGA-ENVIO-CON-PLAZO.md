# Envio con espera de espacio

Base 9709efd. Integracion principal amplia solo API kernel:
nxk_tarea_enviar_esperando(destino,valor,ticks). Retorna 0 si encola,
1 si vence plazo o consulta llena con cero ticks, -1 por contrato/ID invalido,
-3 si el destino ya es terminal. Estado ESPERANDO_ENVIO agregado al final.
No modifica NXE, INT 0x81 ni el cambio NEXTRY ASM. Puente C cooperativo x86.

## Funcionamiento

Cuando el buzon esta lleno, el emisor se marca ESPERANDO_ENVIO y guarda el
valor en su propia pila, sin ocupar turnos ni reservar un lugar en el destino.
Recibir un mensaje publica el hueco y despierta los emisores que esperan ese
buzon. Compiten en las rondas normales; quien no consiga espacio vuelve a
esperar con el plazo original. Reintentar no amplifica el plazo indefinidamente.

Terminar o cancelar el destino despierta a sus emisores, que devuelven -3.
Si el destino se libero antes de reanudar, el ID deja de ser valido (-1).
Reutilizar el slot no permite entregar el mensaje a otra generacion: cada
intento busca por ID completo. Cancelar un emisor esperando descarta su envio
pendiente; no deja mensajes ocultos para entregar mas tarde.

Cero ticks consulta sin bloquear. Se aceptan plazos 1..2^31-1. La funcion
intenta enviar antes de comprobar vencimiento: si hay hueco al reanudar, el
espacio disponible tiene prioridad, igual que el mensaje disponible al recibir.
No es un plazo estricto de tiempo real. Esta variante solo admite tarea actual
con IF habilitada; la funcion enviar no bloqueante sigue admitiendo origen 0
fuera de tarea. Dormir no despierta anticipadamente por estos eventos.

La consulta de tarea muestra el ID del destinatario en esperando; la consola
reconoce el estado esperando_envio. No se cambia el orden FIFO de mensajes ya
encolados. No hay reserva de hueco ni FIFO estricto de emisores bloqueados.
La ampliacion ENTREGA-CICLOS-CRUZADOS.md detecta ciclos entre envio bloqueado
y espera de finalizacion. Los protocolos arbitrarios de envio/recepcion siguen
fuera del alcance del detector.

## Demostracion y pruebas

`tareas flujo` y `tareas mensajes` usan el mismo intercambio de 16 mensajes,
ahora con espera en ambos extremos. La configuracion inicial de la prueba
comprueba que el receptor ya espera antes de enviar el primer mensaje.

Autopruebas del kernel: buzon de cuatro prellenado, quinto mensaje esperando,
despertar al vaciar, origen/orden correctos, vencimiento sin insertar nada,
cero ticks lleno, plazo invalido y llamada fuera de tarea. Se verifica
cancelacion del destinatario, liberacion/reutilizacion con otro ID y cancelacion
del emisor sin ejecutar su continuacion ni publicar el valor pendiente.

Dos productores intercambian 32 valores con un consumidor mediante las dos
operaciones con espera; se comprueba origen, orden por emisor y cantidades.
Los lotes se repiten sin crecimiento de heap o numero de guardas.

Sigue siendo cooperativo, una CPU, ring0 y puente C; no es una llamada de
sistema para apps ni una nueva instruccion NEXTRY. No usar desde IRQ.

Verificacion ejecutada: matriz QEMU x86 32/64/128/1024 MiB, regresion x64
64 MiB, consola del planificador/general y dieciseis escenarios NXE2. La
consola repite tareas flujo dos veces y comprueba memoria estable; NEXTRY
protegido sigue ejecutando despues. API nueva x64 retorna -64.
