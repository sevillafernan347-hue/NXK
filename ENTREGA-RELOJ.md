# Temporizador x86 inicial

Base: d055bbc. Puente C con entrada propia en kernel/reloj32.nxa, ensamblada por NEXTRY ASM.
PIC remapeado a 32/40, solo IRQ0 habilitada, PIT aproximadamente 100 Hz.
El manejador conserva registros generales y pila, cuenta ticks y envia EOI.
El comando tiempo consulta el contador mientras la consola sigue activa.

Prueba: test.ps1 -Architecture x86 -MemoryMatrix superada con 32, 64, 128 y
1024 MiB. Todos los registros muestran RELOJ IRQ OK tras cinco ticks y HLT,
y BOOT PASS despues de las pruebas de memoria y cargadores.
Imagen normal reconstruida. No prueba hardware fisico.

Limites: solo x86, una tarea, sin planificador, contador de 32 bits con vuelta
aproximadamente cada 497 dias; no mide hora civil. Teclado sigue por sondeo.
Las aplicaciones ring0 pueden deshabilitar interrupciones. IRQ distintas de 0
permanecen enmascaradas. La entrada asume segmentos y pila validos del kernel.

Migracion: instrucciones recuperar_registros32 y retornar_interrupcion32 agregadas
al ensamblador. La plantilla comprueba longitud y posiciones antes de instalarse.
Pruebas de regresion x86 superadas con 32/64/128/1024 MiB tras la migracion.
