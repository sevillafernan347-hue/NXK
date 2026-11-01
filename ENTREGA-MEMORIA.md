# Entrega de la cuenta principal: paginas fisicas

Base: bc11d0336269e92d85066eede5e8f2732158038e.
Rama: principal/paginas-fisicas. Responsable: cuenta principal e integracion.
Archivos previstos: kernel/paginas.c, kernel/pruebas_paginas.c, lib/paginas.h,
kernel/main.c, build.ps1, test.ps1 y documentos de estado.

Contrato aprobado por la cuenta principal: paginas de 4096 bytes, limite inicial
1 GiB, reserva conservadora de los primeros 8 MiB. Los metadatos ocuparan
0x600000..0x610000, dentro de esa reserva, solo si BIOS confirma RAM utilizable.
No cambian la API NXE ni el trabajo del compilador de la segunda cuenta.
Implementacion puente en C; pendiente migrar a NEXTRY cuando admita punteros.

## Resultado de la entrega

Implementado y probado. La reserva de paginas devuelve memoria sin inicializar;
el llamador debe borrarla antes de exponerla a programas. No se reemplazo el heap
incremental y no se cambio la API de NXE. No hay sincronizacion para varias CPU.

Pruebas ejecutadas: test.ps1 -Architecture x86 -MemoryMatrix (32, 64, 128,
1024 MiB), test.ps1 -Architecture x64 (64 MiB, gestor deshabilitado).
Los casos sinteticos incluyen rangos de RAM duplicados, reserva parcial sobre
RAM, desbordamiento de direcciones y reserva sobre los metadatos. Se escriben
dos paginas reales y se agotan/liberan todas las paginas del mapa real.

Archivos para estudiar: kernel/paginas.c, kernel/pruebas_paginas.c, lib/paginas.h.
El comando mem muestra el numero de paginas libres y el total administrado.

Una propuesta externa (`memoria-A.diff`, adjunta por el usuario) detecto que
E820 puede dividir en varias regiones contiguas los 64 KiB de metadatos. Se
integro su validacion pagina a pagina y pruebas de region partida y hueco; la
autoría del cambio y sus pruebas se verifican en este historial.
