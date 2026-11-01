# Primera prueba de privilegios x86

GDT amplia los segmentos existentes con codigo DPL3 (selector 43) y datos
DPL3 (selector 51), manteniendo los indices de ambas TSS. TSS normal recibe
SS0/ESP0 para que la CPU use una pila del kernel al cambiar de privilegio.
Dos paginas del heap son accesibles al usuario en una prueba aislada: codigo
solo lectura y pila escribible. El resto de PTE conserva supervisor; PDE permite
usuario solo para resolver esas entradas. Se recarga CR3 para invalidar TLB.
IRET transfiere a CPL3 con IOPL0 e IF apagada. INT 0x80 tiene DPL3 y entra en
el manejador del kernel. Se comprueba CS guardado con RPL3 y CS actual ring0.

Pruebas separadas: 414 INT 0x80 autorizado; 514 CLI rechazada con #GP(0);
614 escritura a 0x10000 rechazada con #PF(7), CR2 correcto y CS usuario.
Se conservan los siete casos anteriores de excepciones. Las pruebas terminan
en el manejador fatal y dispositivo de salida QEMU; no vuelven al programa.

Limitaciones: solo infraestructura y prueba, no procesos utilizables. Comparte
CR3 con kernel, no hay cambio de espacios ni validacion de llamadas publicas,
no carga NXE como usuario, no preserva/restaura mapeos al terminar (es una
imagen de prueba que finaliza). IRQ deshabilitadas en usuario porque las entradas
actuales aun no normalizan segmentos para ese escenario. No hay preempcion,
NX, aislamiento entre usuarios ni recuperacion del fallo. Las apps habituales
siguen en ring0. Implementacion C/asm puente pendiente de NEXTRY ASM.

Validacion: diez escenarios de excepciones/transiciones, matriz x86 32/64/128/1024 MiB y regresion x64 64 MiB superados.
