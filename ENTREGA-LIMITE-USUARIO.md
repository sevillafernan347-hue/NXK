# Limite de ejecucion de usuario por PIT

El puente NEXTRY ASM entra a CPL3 con IF=1 e IOPL=0. IRQ0 conserva registros
y DS/ES/FS/GS, normaliza segmentos para C y recibe el contexto interrumpido.
Se envia EOI antes de una eventual salida anticipada. Tras diez ticks desde
inicio de usuario, si CS guardado es CPL3 y hay ejecucion activa, aborta la
prueba por su pila kernel y devuelve consola. El historial usa vector 32 para
este evento IRQ/PIT, no una excepcion CPU; error/direccion cero.

Comando usuario bucle: EB FE nunca sale voluntariamente. El temporizador lo
termina sin que ceda. usuario fallos ahora verifica nueve casos; despues del
limite se ejecuta el programa correcto y las tareas siguen recibiendo ticks.
Diez ticks son aproximadamente 100 ms a la frecuencia actual, no tiempo real
garantizado. No planifica varios procesos ni suspende/reanuda el programa.
Los bucles del kernel o NXE ring0 siguen sin limite preventivo.

Construccion: huellas de reloj/tareas/entrada/puentes invalidan envoltorios
incbin de Zig. Verificador compara diez binarios NXA con sus bytes ELF reales.
Se detecto durante desarrollo un objeto de IRQ viejo pese a fuente modificada;
la comprobacion de plantilla detuvo el arranque. Corregida la invalidacion.

Pruebas: seis casos unitarios del ensamblador, matriz x86 32/64/128/1024 MiB,
consola serie con usuario bucle y recuperacion, regresion x64 64 MiB. El soporte
nuevo es x86; C conserva politica y temporizador, entrada IRQ/usuario es NXA.
Sigue siendo demostracion controlada y no ejecutor de binarios desconocidos.
