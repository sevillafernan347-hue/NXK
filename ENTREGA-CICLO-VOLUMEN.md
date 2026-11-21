# Estado y desmontaje de NXFS

Rama `kernel/volumen-ciclo-v037`, base `c6a593e`. Alcance: lector NXFS,
consola, pruebas host y QEMU, y documentación de la entrega.

## Contrato

- `volumen estado` informa si NXFS está montado y, si lo está, cuántos
  archivos contiene. Un volumen vacío se muestra como montado con cero
  archivos. La consulta solo usa memoria.
- `volumen desmontar` retira el catálogo y descarta el estado de lectura
  secuencial. Se puede repetir. El buffer de verificación que ya se hubiera
  reservado se conserva para reutilizarlo. No se accede al disco ni se modifica
  el diagnóstico ATA, los archivos RAM o los procesos ya cargados.
- `volumen montar` vuelve a identificar el disco y validar la cabecera.
  El catálogo se publica tras completar la validación. Si el montaje falla,
  el volumen queda desmontado, igual que antes se descartaba su catálogo.
- API interna del kernel: `nxk_volumen_estado()` devuelve -1 si está
  desmontado o la cantidad de archivos (0 a 8). `nxk_volumen_desmontar()`
  pasa al estado desmontado. Los punteros prestados por
  `nxk_volumen_nombre()` dejan de ser válidos al montar o desmontar.

Sigue siendo un puente C, con una imagen NXFS1 de solo lectura en IDE
primario esclavo. Desmontar retira el acceso por NXFS en el kernel. No
desconecta ni apaga el dispositivo. Las operaciones del volumen usan
la ejecución serializada actual del kernel.

## Uso desde la consola

Si ya preparaste la imagen de datos, conéctala con
`./run.ps1 -Architecture x86 -Volumen build/x86/datos-nxfs.img`. Usa estos
comandos desde la consola de NXK para consultar, montar y retirar el volumen.

```text
volumen estado
volumen montar
volumen estado
volumen listar
volumen desmontar
```

## Pruebas realizadas

- `test.ps1 -Architecture x86 -MemoryMatrix` pasó en QEMU con BIOS y
  32, 64, 128 y 1024 MiB. Las pruebas host incluidas confirmaron 141
  comprobaciones ATA y el ciclo NXFS, incluida la invalidación del buffer
  al desmontar y remonte tras un fallo de cabecera.
- `test.ps1 -Architecture x64` pasó como regresión de arranque con 64 MiB.
  El lector NXFS sigue disponible en x86.
- `lenguaje/pruebas_volumen.py` pasó 20 escenarios en QEMU, incluidos el
  desmontaje repetido, el volumen vacío, la conservación de archivos RAM
  y el uso del mismo buffer de 4 KiB tras montar de nuevo. Las imágenes
  de datos quedaron intactas.
- `lenguaje/pruebas_usuario_volumen.py` pasó diez escenarios. Una aplicación
  NEXTRY dejó de leer NXFS al desmontar y recuperó el acceso tras montarlo.
