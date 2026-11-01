# Primer volumen NXFS1 de solo lectura

Formato original de imagen: sectores de 512 bytes, cabecera sector 0 con NXFS,
version uint32=1 en 4 y cantidad 0..8 en 8. Tabla desde 16: ocho entradas de
40 bytes, nombre ASCII terminado en cero hasta 23 caracteres; longitud uint32
en 24, LBA inicial en 28, FNV1a de contenido en 32, reservado en 36. FNV1a de
los primeros 508 bytes de cabecera en 508. Enteros little-endian. Archivos de
0..4096 bytes, sectores contiguos, al menos uno reservado incluso para vacio.
Se rechazan nombres/intervalos duplicados, sectores fuera del disco y nombres
sin terminador. FNV detecta corrupcion accidental; no autentica.

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
QEMU conecta imagen secundaria con snapshot para no cambiar la base; el
controlador NXK carece de escrituras. En consola: volumen montar, volumen listar,
volumen leer leeme.txt. Los archivos preparados persisten en la imagen del host;
NXK todavia no guarda modificaciones al disco. Los archivos RAM siguen volatiles.

Pruebas: matriz x86, regresion x64 y nueve imagenes NXFS de QEMU: correcta,
cabecera, cantidad, nombre, LBA fuera de disco, solapamiento, tamano, checksum
cabecera y contenido. Lectura de archivo de 1100 bytes cruza sectores; archivo
ausente se rechaza. Help funciona despues de cada caso y bases quedan intactas.
Codigo C puente pendiente de migracion a NEXTRY. Sin prueba de hardware fisico.
