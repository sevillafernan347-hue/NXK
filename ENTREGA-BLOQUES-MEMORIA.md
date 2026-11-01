# Bloques de paginas fisicas

API propia x86, puente C: nxk_paginas_reservar_bloque(cantidad, alineacion)
y nxk_paginas_liberar_bloque(inicio, cantidad). Unidades de 4096 bytes.
Alineacion: potencia de dos en paginas. Busqueda desde el comienzo administrado;
no atraviesa reservas ni paginas ya prestadas. Cada bloque se entrega en cero.
La liberacion valida todas las paginas antes de borrar y cambiar contadores.
Un rechazo no altera el rango. Tambien se borra al liberar.

Pruebas QEMU x86 32/64/128/1024 MiB superadas: alineacion 16 KiB, pagina
reservada intermedia, cantidades cero/desbordadas, alineacion invalida,
liberacion parcial invalida sin cambiar contenido, doble liberacion, borrado,
limite de 1 GiB y fragmentacion con espacio total suficiente. Regresion x64
64 MiB superada; API nueva no disponible alli.

Limites: no sincroniza IRQ/multiples CPU; no llamar concurrentemente. No tiene
identificador de propiedad por bloque: se permite liberar cualquier rango
completamente prestado, aunque provenga de varias reservas. No hay memoria
virtual ni proteccion por proceso. Proporciona una base para buffers, no un
controlador DMA: no garantiza restricciones particulares de dispositivos.
