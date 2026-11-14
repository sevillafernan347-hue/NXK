# Liberacion atomica de paginas fisicas no contiguas

Base bfc8c8b (v0.0.28). Contrato aprobado por integracion para v0.0.29.
Alcance de esta rama: kernel/paginas.c, lib/paginas.h y
kernel/pruebas_paginas.c. Sin cambios de ABI usuario o formato ejecutable.

API kernel nxk_paginas_liberar_lista(const uint32_t *paginas,uint32_t cantidad):
libera de una vez entre 1 y 32 direcciones fisicas de paginas de 4096 bytes.
La lista es un puntero kernel confiable, legible para cantidad entradas, y
puede estar almacenada dentro de una de las paginas que se van a liberar.

Primero copia la lista completa a memoria local. Valida todas las direcciones,
duplicados y pertenencia al gestor antes de borrar una pagina o modificar
mapas, contadores o pista. Tras validar, borra todas las paginas y las devuelve
al asignador; las direcciones no necesitan ser contiguas ni estar ordenadas.

Retornos x86: 0 exito; -1 puntero nulo, cantidad 0 o mayor de 32, direccion
desalineada, fuera del rango administrado o duplicada; -2 pagina no prestada;
-3 gestor sin iniciar. En x64 devuelve -64. Un error no cambia paginas ni
estado del gestor. Solo una CPU, sin concurrencia ni llamadas desde IRQ.

Puente C pendiente de migrar a NEXTRY. La integracion usa esta operacion
para devolver las paginas de un espacio privado sin depender de contiguidad.

Pruebas agregadas: listas de 1 y 32 miembros, lista no contigua/desordenada,
casos invalidos, duplicados,
mezcla de pagina prestada/libre, conservacion de bytes y mapas al rechazar,
lista dentro de pagina liberada, llamada previa al inicio y rechazo x64.
Validado al integrar: matriz QEMU x86 32/64/128/1024 MiB y regresion x64
64 MiB superadas el 14 de noviembre de 2026.
Revision estatica y git diff --check sin hallazgos. Memoria local para copiar
la lista: 32 uint32_t (128 bytes), sin nuevas variables persistentes ni heap.
