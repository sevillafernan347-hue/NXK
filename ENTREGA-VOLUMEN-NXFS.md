# Primer volumen NXFS1 de solo lectura

Formato original de imagen: sectores de 512 bytes, cabecera sector 0 con NXFS,
version uint32=1 en 4 y cantidad 0..8 en 8. Tabla desde 16: ocho entradas de
40 bytes, nombre ASCII terminado en cero hasta 23 caracteres; longitud uint32
en 24, LBA inicial en 28, FNV1a de contenido en 32, reservado en 36. FNV1a de
los primeros 508 bytes de cabecera en 508. Enteros little-endian. Archivos de
0..4096 bytes, sectores contiguos, al menos uno reservado incluso para vacio.
Se rechazan nombres/intervalos duplicados, sectores fuera del disco y nombres
sin terminador. FNV detecta corrupcion accidental; no autentica.

Desde v0.0.32, los bytes reservados 12..15, el relleno tras el terminador
de cada nombre, los cuatro bytes reservados de cada entrada y la zona
posterior a la ultima entrada activa hasta el byte 507 deben ser cero.
El generador oficial siempre escribe esos ceros. El montaje rechaza
cabeceras que no los respeten, incluso si su huella concuerda.
Las imagenes creadas por otros generadores deben respetar el mismo relleno.

Driver lee solo IDE primario esclavo. Montaje valida toda la tabla antes de
publicarla; cualquier montaje fallido invalida el anterior. Lectura usa staging
de 4096 bytes y verifica huella antes de copiar al destino. No imprime contenidos
corruptos. Limite ocho archivos; sin carpetas, escritura, journal ni montaje RAM.
Estas funciones son kernel/console, aun no se exponen a aplicaciones CPL3.

Crear imagen nueva: tools/python/python.exe lenguaje/crear_volumen.py build/x86/datos-nxfs.img
El modo por defecto incluye leeme.txt. Para contenido propio: repetir
--archivo nombre=ruta, maximo ocho y 4096 bytes por archivo. No sobrescribe
salidas existentes. No abre dispositivos fisicos. Iniciar con:
./run.ps1 -Architecture x86 -Volumen build/x86/datos-nxfs.img
QEMU conecta imagen secundaria con snapshot para no cambiar la base. NXFS
solo lee su imagen. En consola: volumen montar, volumen listar,
volumen leer leeme.txt. Los archivos preparados persisten en la imagen del host;
NXK todavia no guarda modificaciones al disco. Los archivos RAM siguen volatiles.

Pruebas: matriz x86, regresion x64 y nueve imagenes NXFS de QEMU: correcta,
cabecera, cantidad, nombre, LBA fuera de disco, solapamiento, tamano, checksum
cabecera y contenido. Lectura de archivo de 1100 bytes cruza sectores; archivo
ausente se rechaza. Help funciona despues de cada caso y bases quedan intactas.
Codigo C puente pendiente de migracion a NEXTRY. Sin prueba de hardware fisico.

## Validacion de la cabecera en v0.0.32

En la base 6ea7e69, QEMU monto una imagen con el byte 15 distinto de cero
y una huella de cabecera recalculada. Con la correccion, el kernel rechazo
esa imagen y cuatro variantes equivalentes en el campo reservado de la
ultima entrada, el relleno del nombre, una entrada inactiva y el ultimo
byte de relleno de la cabecera. Ninguno de los montajes rechazados
permitio leer su primer archivo.

El 17 de noviembre de 2026, la matriz de `test.ps1 -Architecture x86
-MemoryMatrix` paso con 32, 64, 128 y 1024 MiB. En la imagen normal,
`lenguaje/pruebas_volumen.py` paso los 16 escenarios en QEMU y verifico
que los discos secundarios de prueba conservaran sus bytes. Entre las
imagenes aceptadas se incluyeron un volumen vacio y otro con las ocho
entradas ocupadas y un nombre de 23 caracteres.

La propuesta 0.0.37 agrega `volumen estado` y `volumen desmontar`. El procedimiento
y las pruebas se describen en [ENTREGA-CICLO-VOLUMEN.md](ENTREGA-CICLO-VOLUMEN.md).
