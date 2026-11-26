# Verificación NXFS entre turnos

Rama `kernel/verificacion-turnos-v041`, base `693568f`. Alcance:
verificación NXFS, planificador CPL3, consola x86, pruebas y notas.

## Contrato

`volumen iniciar NOMBRE` prepara una verificación de un archivo NXFS
montado. Solo hay una tarea activa. El inicio no lee sectores. Los
archivos vacíos se verifican en el acto con la huella de longitud cero.

Tras cada turno real de un proceso CPL3 gestionado, el kernel procesa
hasta un sector del archivo. `volumen paso` permite hacer un paso manual
cuando no se ejecutan procesos. `volumen progreso` consulta los bytes
procesados y el resultado sin leer del disco. `volumen cancelar` detiene
una tarea pendiente y conserva el progreso para consulta.

El paso usa un búfer de 512 bytes en la pila y acumula FNV1a sobre la
longitud real del archivo. No publica contenido ni altera el búfer de
la ruta síncrona. Desmontar, volver a montar o detectar un cambio en el
disco descarta la tarea pendiente. Si se alcanza el límite de revisiones
del disco, no se inicia una nueva verificación por turnos con contenido.
Volver a identificar la unidad con `discos` también cambia su revisión.
`discos estado` consulta el diagnóstico sin cambiarla.

La API interna es aditiva y no cambia la ABI de aplicaciones. La
verificación por turnos es un puente C para x86. El sector individual
sigue usando ATA PIO síncrono.

## Uso desde la consola

Después de `volumen montar`, ejecuta `volumen iniciar nombre` con un
archivo listado en NXFS. Para ver cuánto ha avanzado, usa
`volumen progreso`. Al ejecutar procesos también avanza la tarea.
`volumen paso` permite continuarla desde la consola.

## Pruebas realizadas

- El lector con disco simulado verificó un sector por llamada, archivos de
  5 y 4096 bytes, preservación de la copia secuencial, errores ATA y cambios
  de disco, reinicio, desmontaje y cancelación.
- Pasaron 22 escenarios NXFS en QEMU. Un paso manual llegó a 512 de 4096
  bytes. Dos procesos hicieron seis turnos y la tarea avanzó a 3584 bytes.
  Un paso manual la completó. También se comprobó la huella incorrecta en
  el último sector y el caso vacío sin ninguna lectura.
- Pasaron los diez escenarios de lectura NXFS desde aplicaciones NEXTRY.
- Arranque BIOS x86 comprobado en QEMU con 32, 64, 128 y 1024 MiB, con
  166 comprobaciones ATA simuladas. Regresión x64 con 64 MiB. La ASUS
  física sigue pendiente.
