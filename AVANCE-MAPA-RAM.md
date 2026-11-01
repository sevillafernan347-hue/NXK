# Mapa de RAM del BIOS

El cargador de 32 bits consulta INT 15h E820 antes de activar el modo protegido.
La implementacion esta en NEXTRY ASM y admite hasta 32 registros de 20 bytes.
Un mapa truncado o una firma incorrecta no se publica como valido.

La cabecera se guarda en 0x5000: cantidad (16 bits) y valido (16 bits).
Los registros empiezan en 0x5010. Esta zona debe reservarse al crear el gestor
de paginas. No se cruza con la pila, el kernel ni el sector BIOS del prototipo.

`kernel/ram.c` suma regiones de tipo 1 por debajo de 4 GiB. Es un puente C
pendiente de migrar. El valor es RAM utilizable descrita por BIOS, no memoria
libre: incluye zonas que el kernel ocupa. No usa ese mapa para asignar paginas.
La variante x64 devuelve cero porque su arranque aun no recoge el mapa.

Pruebas: QEMU x86 con 32, 64, 128 y 1024 MiB; se compara el valor detectado
con la RAM configurada, admitiendo 2 MiB para regiones reservadas. Falta probar
firmware fisico y mapas solapados; no usar aun para asignacion de paginas.

Referencia del protocolo:
https://uefi.org/specs/ACPI/6.5/15_System_Address_Map_Interfaces.html
