# Paginacion y proteccion inicial x86

Implementacion propia C en kernel/paginacion.c, pendiente migracion a NEXTRY.
Tablas de 4 KiB: directorio 0x200000 y 256 tablas desde 0x201000, final 0x301000.
Esta zona queda dentro de los primeros 8 MiB excluidos del asignador. Antes de
escribir se comprueba cada pagina contra BIOS E820, rechazando solapamientos
reservados, huecos y desbordamiento. Coste fijo: 1028 KiB de RAM.

Mapeo identidad del primer GiB, solo supervisor. Pagina cero ausente. Codigo y
rodata del kernel alineados al final de pagina y sin escritura; CR0.WP activo
aplica la proteccion tambien a ring0. Datos, heap, pilas y entradas generadas
siguen escribibles. Los nombres de simbolos nuevos no alteran ABI NXE.
El manejador de pagina informa CR2 ademas del vector, error y EIP.

Validacion: matriz QEMU x86 32/64/128/1024 MiB; pruebas provocadas INT3, UD2,
proteccion general, pagina nula (vector 14/error 2/CR2 cero) y escritura de codigo
(vector 14/error 3/CR2 en kernel). Regresion x64 con 64 MiB.
El selector de prueba 114 representa escritura protegida, no un vector CPU.

Limites: no asigna direcciones virtuales nuevas ni cambia espacios por proceso;
no hay ring3, aislamiento, intercambio a disco ni permisos NX. Mapea direcciones
que no necesariamente son RAM: no deben usarse sin validar su mapa fisico.
Las tablas son escribibles en ring0, por lo que no protege contra codigo malicioso
privilegiado. Un fallo detiene el kernel; no recupera procesos. x64 conserva su
arranque anterior, no usa esta implementacion. Hardware fisico aun sin probar.
