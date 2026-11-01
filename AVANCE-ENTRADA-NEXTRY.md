# Entrada principal x86 migrada a NEXTRY ASM

El kernel de 32 bits entra mediante `kernel/entrada32.nxa`: limpia la seccion de variables BSS y llama a `kernel_main`. Nuestro ensamblador genera directamente sus 31 bytes. `entry32.S` se conserva como referencia, pero ya no proporciona las instrucciones de esa entrada en la construccion x86.

Nuevas instrucciones: `cero eax`, `restar32 ecx, edi` y `llenar_bytes` (REP STOSB). Se agregaron simbolos externos mediante `externo` y un archivo JSON de direcciones.

La construccion hace un enlace provisional, extrae `__bss_start`, `__bss_end` y `kernel_main`, vuelve a ensamblar con las direcciones resueltas y comprueba que el enlace final conserva esas direcciones. El objeto provisional no se entrega como kernel final. Las huellas en los envoltorios de datos evitan reutilizar objetos de cache con binarios embebidos anteriores.

Se conservan dependencias externas: los bytes se incorporan a un objeto mediante un envoltorio de datos y LLD enlaza el kernel. El ensamblador propio sigue implementado en Python. No se ha sustituido aun el compilador C ni el enlazador externo.

## Estado de la migracion x86

- Arranque BIOS: NEXTRY ASM (`boot/arranque32.nxa`).
- Entrada principal: NEXTRY ASM (`kernel/entrada32.nxa`).
- Entrada fatal de excepciones: NEXTRY ASM (`kernel/excepciones32.nxa`).
- Puente de retorno de EXE: ensamblador anterior, pendiente.
- Memoria, consola, tablas y cargadores: mayormente C, pendientes.

Pruebas: arranque y ejecucion de los ejemplos con 32, 64, 128 y 1024 MiB; excepcion INT3 capturada. La variante x64 no se migra en esta etapa.
