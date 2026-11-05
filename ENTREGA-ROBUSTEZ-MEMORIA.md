# Robustez de memoria BIOS y archivos RAM

Base v0.0.5, commit 4dfd127. Entrega v0.0.6.

El BIOS puede describir RAM util con rangos contiguos o solapados y con cortes
no alineados. El gestor de paginas une esos rangos antes de exigir paginas
completas. Los metadatos en 0x600000..0x610000 deben quedar totalmente cubiertos;
un hueco de un byte basta para rechazar el mapa. Cualquier region reservada
sigue excluyendo las paginas que toque. Solo se escriben metadatos despues de
validar el mapa. El limite del asignador permanece en 1 GiB.

El indicador de RAM BIOS cuenta la union de RAM util menos regiones reservadas,
sin contar solapamientos dos veces, por debajo de 4 GiB. Es memoria descrita,
no memoria libre: incluye zonas usadas por NXK. Un rango con overflow invalida
el mapa. La rutina x64 todavia no recibe un mapa BIOS.

La API kernel `nxk_archivo_truncar(nombre,longitud)` ajusta un archivo RAM
existente entre 0 y 256 bytes. Al recortar borra el tramo anterior; al ampliar
rellena con ceros. Rechaza nombres invalidos (-1), ausencia (-2) y longitud
excesiva (-3) sin modificar el archivo. No escribe en disco y no expone una
nueva llamada a aplicaciones NXE o una palabra NEXTRY.

Pruebas incluidas en el arranque de pruebas: fragmentos desordenados, reserva
superpuesta, hueco de un byte, mapa duplicado, reserva solapada, overflow y
limite de 4 GiB; archivos recortados, ampliados, vacios y errores. La matriz
QEMU no certifica la laptop fisica. Son puentes C de kernel; quedan pendientes
procesos completos, persistencia, controladores y migracion a NEXTRY.
