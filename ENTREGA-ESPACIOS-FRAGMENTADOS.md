# Espacios privados sobre memoria fisica fragmentada

Base bfc8c8b (v0.0.28). Contrato aprobado por integracion para v0.0.29.
Alcance: kernel/espacios.c, lib/espacios.h, pruebas kernel y sus enlaces;
version, inventario y documentacion. Sin cambios de interfaz ni lenguaje.

Cada espacio conserva cinco paginas base: directorio, tabla baja, tabla alta,
codigo y pila. Se adquieren individualmente; no necesitan direcciones fisicas
consecutivas. Las direcciones virtuales y el contrato publico no cambian.
nxk_espacio_info sigue exponiendo las direcciones reales de directorio,
codigo y pila por separado. No se puede deducir ninguna sumando a otra.

Crear adquiere las cinco paginas antes de publicar el slot, ID o generacion.
Si una reserva falla, devuelve todas las temporales, deja intacta la salida
y retorna -4. Crear con exito copia el codigo, prepara las tablas y publica
una nueva generacion. No consume heap. Destruir valida y libera las paginas de
datos y las cinco paginas base mediante una lista atomica; conserva las prohibiciones
de destruir un directorio activo o retenido por procesos.

Integracion aprueba nxk_paginas_liberar_lista: de 1 a 32 direcciones fisicas,
copia local y validacion completa de rango, pertenencia y duplicados antes de
borrar o publicar cambios. -1 contrato/duplicado, -2 pagina no prestada,
-3 no iniciado, -64 x64. No debe ampliar el mapa ni la reserva fija de metadatos.
Destruir usa una sola lista de hasta nueve paginas; un error conserva el espacio.
Liberar lotes de datos privados reutiliza esta validacion atomica: un registro
fisico invalido al final no libera paginas anteriores ni cambia sus PTE.

Las tablas siguen preservando guardas kernel, codigo RO y pila RW. Los alias
fisicos siguen siendo supervisor. Las copias cruzadas resuelven cada pagina
virtual a su respaldo real. Cuotas, proteccion de datos, IDs y ABI NXE quedan
compatibles. Puente C, una CPU, sin concurrencia/IRQ/SMP ni memoria compartida.

## Pruebas ejecutadas

El 14 de noviembre de 2026, `./test.ps1 -Architecture x86 -MemoryMatrix`
supero QEMU con 32, 64, 128 y 1024 MiB; codigo de salida 33 y marcadores
internos completos. `./test.ps1 -Architecture x64` supero 64 MiB.

- En 32 MiB se retuvo toda la RAM asignable y se dejaron cinco huecos aislados.
  El diagnostico confirmo cinco paginas libres y mayor bloque de una pagina.
  Crear, consultar, copiar, activar y destruir funcionaron con esos respaldos.
- Con 0..4 huecos, cada intento devolvio -4, con salida centinela intacta,
  mismo diagnostico, slot vacio y sin consumo de generacion.
- Se comprobaron PDE/PTE, permisos supervisor/RO/RW, codigo/pila en cero,
  copia cruzando codigo/pila, escrituras mediante CR3 real y rechazo de
  destruccion del directorio activo.
- Una sexta pagina aislada de datos con RO conservo respaldo y permiso tras
  reactivar dos veces. Destruir borro las seis paginas; reciclar recupero
  cuota inicial y una generacion nueva. El heap y el diagnostico se recuperaron.
- Una inconsistencia inyectada en la ultima pagina de datos rechazo tanto
  liberar el lote como destruir el espacio, conservando registros, bytes
  anteriores y contadores. La inyeccion se restaura antes de continuar.
- Liberacion de listas de 1 y 32 paginas; miembros separados/desordenados,
  duplicados, entradas invalidas, mezcla libre/prestada y lista dentro de la
  primera pagina que se borra. Preinicio y rechazo x64 conservados.

Las pruebas de fragmentacion extrema usan <=8192 paginas libres para evitar
un segundo agotamiento masivo en las configuraciones grandes. La prueba
existente de paginas sigue agotando y recuperando las cuatro configuraciones.
El test fragmentado activa CR3 desde kernel; la ejecucion CPL3 se verifica
por las regresiones normales. Revision estatica independiente sin bloqueantes.

Regresiones externas ejecutadas y superadas sobre la imagen normal:
`pruebas_memoria_procesos.py`, `pruebas_buffers_procesos.py`,
`pruebas_proteccion_procesos.py` y `pruebas_lotes_procesos.py` (carpeta lenguaje).
Cubren ejecucion CPL3, aislamiento, cesion/preempcion, FS/IPC cruzando paginas,
permisos RO/RW reales, retenciones, limites, aplicaciones NEXTRY y recuperacion
de recursos. Los discos originales permanecieron intactos.

Archivo kernel x86 normal81932 B/pruebas114700 B; x64 normal24584 B/pruebas28680 B.
Se conservan los limites BIOS y linker; no certifica hardware ASUS fisico.
