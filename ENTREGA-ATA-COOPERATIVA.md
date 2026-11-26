# Lectura ATA cooperativa

Rama `kernel/ata-cooperativa-v043`, base `975aaaf`. Alcance: controlador
ATA x86, lector NXFS gestionado, atención del canal en reposo y pruebas.

## Contrato

La lectura de un sector de un proceso gestionado avanza en varios pasos.
Cada paso admite hasta 32 sondeos del registro de estado. Además usa las
pausas del protocolo al cambiar de fase y transfiere 256 palabras por PIO
cuando DRQ está listo. El controlador conserva el progreso,
el plazo PIT y el límite de sondeos por fase. Un destino del kernel se
entrega a cada paso y solo recibe datos tras el cierre del comando.

El canal primario conserva un solo comando activo. Al cancelar una
solicitud, se descarta el sector. Si se emitió la orden, el controlador
termina de atenderla sin entregarla. Otras operaciones síncronas pueden
cerrar primero una lectura pendiente y NXFS puede retomar su sector
después. Un fallo con BSY o DRQ todavía activo exige recuperar el canal.
Se conserva el contrato de aplicaciones y el formato NXFS1.

`proceso ejecutar N` admite de 1 a 1024 turnos. Si solo quedan procesos
esperando disco, cada sondeo en lote consume un turno del presupuesto.
El valor predeterminado sigue siendo 32. `discos estado` muestra
`pendiente` durante una orden por pasos y `descartada` si se cerró por
cancelación. La consola también drena órdenes descartadas mientras espera
entrada. La verificación manual de NXFS y las demás operaciones ATA usan
la ruta síncrona. El controlador sigue usando sondeo y una sola orden activa.

El cambio es un puente C para BIOS x86. El protocolo de entrada se
contrastó con la sección 11.5 del [borrador ATA/ATAPI-7 de T13](https://citeseerx.ist.psu.edu/document?doi=1856ede6b65d0b38a0d7b6a02f8a3f2a0a3afe67&repid=rep1&type=pdf).

## Pruebas

- Pasaron 181 comprobaciones con el controlador real y puertos simulados.
  Incluyen espera antes de emitir y durante la lectura, tope de 32 sondeos,
  destino intacto hasta el cierre, cancelación, regreso de la API síncrona,
  vencimiento del PIT entre pasos y recuperación por SRST.
- QEMU verificó el presupuesto configurable, dos procesos esperando disco
  mientras un tercero avanzaba, la protección de dos páginas y el descarte
  sin publicar datos. También pasaron las regresiones de procesos, los 22
  escenarios NXFS, los diez casos de lectura desde NEXTRY y las pruebas de
  persistencia y de diagnóstico ATA.
- El arranque BIOS x86 pasó con 32, 64, 128 y 1024 MiB virtuales. La regresión
  x64 pasó con 64 MiB. Falta probar en la ASUS física.
