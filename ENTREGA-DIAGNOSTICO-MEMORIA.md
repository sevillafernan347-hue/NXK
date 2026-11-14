# Diagnostico de paginas fisicas

Base 980f61a (v0.0.27). Integracion aprueba este contrato para v0.0.28.
Alcance: kernel/paginas.c, lib/paginas.h y pruebas internas de memoria
(incluida la comprobacion previa al inicio en kernel/main.c);
version y documentacion de la entrega. Sin cambios de interfaz o lenguaje.

API del kernel nxk_paginas_diagnostico(&salida), puente C pendiente de
migrar a NEXTRY. Devuelve 0 al publicar una instantanea completa; -1 por
puntero nulo, -3 antes de inicializar, -2 por incoherencia de mapas/contadores
y -64 en x64. Un error conserva la salida. El llamador proporciona memoria
kernel escribible y no solapada con los metadatos del asignador.

Todos los campos son uint32_t y cuentan paginas de 4096 bytes:

- total: paginas inicialmente utilizables del mapa aceptado, tras las reservas
  BIOS y la exclusion de los primeros 8 MiB, con limite de 1 GiB.
- libres: paginas disponibles segun el bitmap.
- prestadas: paginas entregadas a consumidores del kernel segun el bitmap.
- excluidas: paginas retiradas por reservar_region despues de inicializar.
  No incluye huecos del BIOS ni RAM inexistente: total - libres - prestadas.
- mayor_bloque: longitud de la mayor secuencia fisica libre. No garantiza
  satisfacer una alineacion mayor de una pagina.
- bloques_libres: numero de secuencias libres separadas por paginas no libres.

La consulta recorre los mapas, rechaza bits libre/prestada simultaneos,
contrasta los contadores y valida la resta antes de publicar. No reserva,
libera, borra ni modifica paginas o la pista de busqueda. Solo una CPU,
en contexto kernel sin concurrencia con el gestor, nunca desde IRQ.
Es una consulta explicita O(numero de paginas hasta 1 GiB), no parte del
camino de asignacion. No compacta memoria ni agrega una llamada de usuario.

## Pruebas ejecutadas

El 14 de noviembre de 2026 se superaron:

- `./test.ps1 -Architecture x86 -MemoryMatrix`: QEMU con 32, 64, 128 y
  1024 MiB. Codigo de salida 33 y todos los marcadores internos correctos.
- RAM continua, exclusiones BIOS que no cuentan como excluidas posteriores,
  reservas parciales/repetidas, fragmentacion, bloques alineados y operaciones
  rechazadas sin modificar el resumen. Consultas repetidas sin efectos.
- Inyeccion controlada de bits libre/prestada simultaneos y discrepancia del
  contador de paginas libres: rechazo -2 y salida intacta. Cada inyeccion se
  restaura antes de continuar; no se incluye en la imagen normal.
- Puntero nulo, llamada previa al inicio, mapa valido sin paginas asignables,
  corrida que llega al limite de 1 GiB y exclusion de esa ultima pagina.
- Agotamiento real del asignador BIOS, resumen sin bloques libres y recuperacion
  completa de todas las paginas y las corridas originales.
- `./test.ps1 -Architecture x64`: arranque en 64 MiB y rechazo explicito -64
  sin modificar la salida. El gestor fisico sigue disponible solo en x86.
- `tools/python/python.exe lenguaje/pruebas_memoria_procesos.py`: regresion
  de aplicaciones CPL3 superada, incluidos cupo, paginas en cero, limites,
  aislamiento, cesion/preempcion, fallos contenidos y recuperacion de recursos.
  Revision independiente del diff sin hallazgos bloqueantes.

Imagen x86 normal 81932 B, pruebas 110604 B; x64 normal 24584 B, pruebas
28680 B. Se conservan los limites BIOS, linker y mapa de memoria; no se agrega
memoria permanente al gestor. Esta API no certifica toda la RAM ni el hardware
ASUS y no garantiza detectar cualquier clase de corrupcion.
