# Verificación de archivos NXFS

Rama `kernel/verificar-nxfs-v038`, base `6e77d42`. El cambio abarca el
lector NXFS, la consola, las pruebas y las notas de entrega.

## Contrato

En x86, tras `volumen montar`, ejecuta `volumen verificar`. NXK lee cada archivo
del catálogo directamente desde la imagen NXFS y compara su huella con la
registrada al montar. Informa qué archivos pasan, distingue fallos de
lectura de huellas incorrectas y cuenta cuántos pasan. Revisa las entradas
restantes aunque una falle.

La API del kernel `nxk_volumen_verificar(indice)` devuelve cero si la huella
coincide, -2 si la entrada no existe, -4 si falta memoria para el buffer,
-5 ante un fallo de lectura ATA y -6 si la huella no coincide. Reutiliza el
verificador y su buffer de 4 KiB. Descarta la lectura secuencial anterior
al verificar una entrada y no entrega bytes al llamador. Los archivos
vacíos se comprueban con la huella de longitud cero sin leer su sector.

El recorrido usa solo lectura y llega a un máximo de 64 sectores de
contenido. La huella FNV1a detecta corrupción accidental. El catálogo
se valida al montar y la verificación de archivos usa ese catálogo.
Sigue siendo un puente C y una operación síncrona.

## Pruebas realizadas

- El lector real con disco simulado distinguió falta de memoria, error ATA
  y huella incorrecta. Comprobó una entrada inválida y una lectura nueva
  aun cuando el archivo tenía una copia verificada para el siguiente tramo.
- Pasaron los 20 escenarios NXFS de QEMU. Se verificaron volúmenes sanos,
  vacíos y desmontados, y discos con un archivo dañado entre otros válidos.
  También se comprobó el rechazo de un fallo en el último sector de un
  archivo de 4 KiB, el tamaño del buffer y las imágenes base intactas.
- `test.ps1 -Architecture x86 -MemoryMatrix` pasó con 32, 64, 128 y
  1024 MiB, con 141 comprobaciones ATA host. La regresión x64 pasó con
  64 MiB. MINI NXK pasó su prueba de pantalla, teclado y consola.
