# Recuperación manual del canal IDE

Rama `kernel/reinicio-ata-v040`, base `2604e1f`. Alcance: controlador
ATA PIO, catálogo NXFS, consola x86, pruebas y documentación.

## Contrato

`discos reiniciar` solicita un reinicio por software de ambas unidades del
canal IDE primario. Requiere un reloj PIT en marcha. Al emitir SRST, el
kernel descarta las capacidades anteriores y la copia secuencial de NXFS.
NXFS desmonta el catálogo al detectar el cambio de generación, incluso
si el reinicio falla. Los archivos RAM siguen disponibles.

El reinicio mantiene las interrupciones ATA deshabilitadas. Espera dos
avances del PIT antes de afirmar SRST, dos con SRST activo y dos tras
liberarlo antes del primer sondeo. El reloj ofrece una espera mínima de
aproximadamente 10 ms en cada intervalo con el PIT a unos 100 Hz.
La comprobación del maestro admite hasta 32 segundos con el reloj activo.
Si el reloj deja de avanzar, la espera termina por un límite de iteraciones.

El resultado de reinicio muestra el estado, el diagnóstico y la firma del
maestro cuando se pueden leer. Si el maestro no queda operativo, la
identificación queda bloqueada hasta completar otro reinicio. Ante un
resultado exitoso, `discos` vuelve a identificar las unidades y
`volumen montar` reconstruye el catálogo NXFS desde el disco secundario.
Un reinicio de canal no recupera ni vuelve a enviar escrituras que hayan
fallado antes. Si el PIT se detiene antes de emitir SRST, se conserva el
estado anterior. La salida distingue ese caso de un fallo tras emitirlo.

## Uso desde la consola

Tras un fallo de lectura, consulta el último diagnóstico con
`discos estado` y ejecuta `discos reiniciar`. Si el reinicio termina,
usa `discos` para comprobar cuáles unidades se pueden identificar.
Para recuperar el acceso a un volumen NXFS, ejecuta `volumen montar`.

## API interna

API de kernel `nxk_ata_reiniciar`: retorna 0 en éxito, -1 por puntero
inválido, -2 si el maestro no responde, -3 por diagnóstico o firma
incompatibles, -4 por espera de estado agotada y -6 si falta avance del PIT.
En x64 retorna -64. Sus campos indican si se emitió SRST y si los registros
de diagnóstico y firma son válidos. La consulta de diagnósticos por unidad
conserva la evidencia anterior. Sigue siendo un puente C para x86 BIOS.

[Referencia de hardware T13/1321D rev. 3, secciones 9.2 y 9.12](https://www.seagate.com/support/disc/manuals/ata/d1153r17.pdf#page=257).

## Pruebas realizadas

- Pasaron 166 comprobaciones ATA con puertos simulados. Cubrieron el reloj
  detenido antes y durante el pulso, BSY persistente, maestro ausente,
  firmas, bloqueo tras un fallo y recuperación posterior.
- En QEMU pasó el reinicio con el secundario ausente. El maestro volvió a
  identificarse y el diagnóstico anterior quedó disponible.
- Pasaron 21 escenarios de NXFS. Se comprobó el reinicio repetido con el
  volumen montado, la conservación de archivos RAM y el acceso a disco
  tras un montaje nuevo. Pasaron 38 comprobaciones de persistencia,
  incluida la recuperación de una instantánea tras reiniciar el canal.
- Se verificó desde NEXTRY en CPL3 que una lectura queda rechazada tras
  SRST y vuelve a funcionar al montar otra vez.
- Arranque BIOS comprobado en QEMU x86 con 32, 64, 128 y 1024 MiB y en
  x64 con 64 MiB como regresión. La ASUS física sigue pendiente.
