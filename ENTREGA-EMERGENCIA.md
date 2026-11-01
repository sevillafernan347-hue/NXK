# Pila independiente para doble fallo x86

Implementacion propia C en excepciones.c. GDT conserva los tres descriptores
BIOS existentes y agrega dos TSS de 104 bytes. LTR registra la tarea normal;
la puerta de tarea IDT 8 activa una TSS fatal con CR3 actual, segmentos kernel,
IF apagada y pila independiente de 4 KiB reservada del heap.
El manejador informa doble fallo y ESP anterior, luego detiene la CPU.
No regresa, no recupera tareas y no reemplaza al planificador cooperativo.

La inicializacion exige la GDT BIOS esperada de 24 bytes. Especifico x86,
una CPU. Se conserva diagnostico normal para excepciones restantes. La nueva
pila no tiene guarda propia y no soporta fallos anidados dentro del manejador.
Sigue siendo puente C pendiente de NEXTRY y hardware fisico sin verificar.

Prueba 314: reserva RAM, marca 0x101000 como guarda, cambia ESP a 0x101004 y
provoca PUSH en la guarda. El intento de entregar la excepcion de pagina falla
sobre esa pila y genera doble fallo. Se comprueba marcador de pila independiente
y salida 37 por dispositivo de pruebas QEMU. No se recupera la ejecucion.
Tambien se ejecutan los seis casos de excepciones anteriores, matriz x86
32/64/128/1024 MiB y regresion x64 con 64 MiB.
