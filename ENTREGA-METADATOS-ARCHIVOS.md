# Consulta de metadatos de archivos

Base f933791. La integracion aprueba dos consultas internas aditivas.
`nxk_archivo_info` busca primero en RAM y luego en el catalogo NXFS montado.
`nxk_volumen_tamano` consulta solo NXFS. Ambas dejan intacta la salida al
rechazar. Usan metadatos en memoria y no reservan heap ni acceden a ATA.
No cambian la ABI de usuario ni el formato del disco.

El comando `archivo info NOMBRE` muestra el origen visible y el tamano en
bytes. `copiar` consulta el destino por nombre en los metadatos, incluso si
su contenido esta corrupto. Conserva sus codigos y verifica todo el origen
antes de publicar una copia en RAM.

Ejemplo: si `leeme.txt` esta en el volumen, la consola responde
`Archivo NXFS: 37 bytes` para un archivo de esa longitud. Si se crea uno
homonimo en RAM, la misma consulta devuelve RAM y su nueva longitud.
Despues de borrarlo, el nombre vuelve a resolver a NXFS.

El catalogo conserva la longitud verificada al montar su cabecera.
Leer o copiar el contenido desde NXFS sigue comprobando su huella.
La consulta puede devolver metadatos de una entrada cuyo contenido falle.
El cambio reduce 256 bytes de buffers locales en la operacion de copia.

## Comprobaciones

- QEMU x86 arranco con 32, 64, 128 y 1024 MiB. La regresion x64 paso con
  64 MiB. La prueba interna comprueba salida intacta al rechazar y longitud
  de archivos RAM vacios y ampliados.
- Pasaron los 19 escenarios NXFS. El comando muestra longitudes de 0 a
  4096 bytes, resuelve RAM sobre NXFS y vuelve a mostrar NXFS tras borrar
  la copia RAM. El ultimo diagnostico ATA siguio indicando la lectura
  de la cabecera durante las consultas y al rechazar copias sobre destinos
  existentes, incluso con contenido corrupto. Consultar y rechazar la
  colision no hicieron crecer el heap.
- Pasaron las 38 comprobaciones de persistencia con discos virtuales.
- Pasaron los diez casos de copia NEXTRY en CPL3. Cubrieron archivos
  vacios, grandes, corruptos, colisiones en RAM y disco y tabla RAM llena.
