# Migracion del puente usuario a NEXTRY ASM

Entrada a ring3, salida INT 0x80 y aborto recuperable expresados en tres fuentes
NXA comentadas. Se elimina kernel/usuario32.S. No cambia el convenio con C:
registros preservados, EFLAGS, segmentos, pila y resultado mantienen significado.
El organizador, TSS/GDT y diagnosticos conservan puentes C.

Simbolos ELF incluye las dos variables de contexto del puente, ademas de los
tres simbolos de entrada. Dos pasadas resuelven referencias; comprobacion final
exige direcciones estables. La huella JSON invalida el objeto al cambiar simbolos.

Pruebas: cinco casos del ensamblador (bytes y rechazos), 19 del compilador,
matriz x86 32/64/128/1024 MiB y regresion x64 64 MiB superadas. Consola serie
verifica errores contenidos, retorno, estado e historial usando el puente NXA.
Esto es migracion de un componente: el kernel aun no es completamente NEXTRY.
