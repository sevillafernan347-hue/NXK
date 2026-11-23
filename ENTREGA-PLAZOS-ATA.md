# Plazos de espera del controlador ATA

Rama `kernel/plazos-ata-v039`, base `1c6c530`. Alcance: controlador y API
interna ATA, diagnóstico de consola, pruebas y notas de la entrega.

## Contrato

Cada fase espera hasta 200 ticks del PIT (unos dos segundos a 100 Hz) si
el dispositivo sigue ocupado o no presenta el estado requerido. El cierre
de FLUSH CACHE admite 1000 ticks (unos diez segundos). El reloj se consulta
cada 32 sondeos pendientes. Cada fase arranca con su propio plazo y la
resta modular conserva el cálculo cuando el contador de 32 bits vuelve a cero.

El límite de un millón de sondeos por fase sigue activo. Se aplica el
primer límite que se detecte. Esto acota la espera aunque las interrupciones
estén deshabilitadas o el reloj aún no esté inicializado.

El resultado de espera agotada sigue siendo -4. La API interna
`nxk_ata_consultar` informa además la causa, que `discos estado` muestra
como `limite PIT` o `limite sondeos`. Ante un fallo se conserva el último
estado leído y la fase. Las lecturas publican el sector tras el cierre.
Las operaciones que escriben siguen sin reintentos automáticos.

El controlador sigue siendo un puente C y espera por sondeo en x86. El
paso a espera por interrupciones y la recuperación del canal se mantienen
como trabajo posterior.

## Resultados

- Pasaron 152 comprobaciones con el controlador real y puertos simulados.
  Cubrieron el reloj detenido, la vuelta a cero del contador, el plazo en
  las tres fases de lectura y el plazo mayor de FLUSH. Una lectura fallida
  dejó intacto el búfer del llamador.
- QEMU arrancó por BIOS en x86 con 32, 64, 128 y 1024 MiB, y en x64 con
  64 MiB como regresión. La consola ATA conservó su diagnóstico entre
  consultas y dejó intacta la imagen de arranque.
- Pasaron los 20 escenarios de NXFS y las 38 comprobaciones de persistencia
  con discos virtuales. La prueba en la ASUS física queda pendiente.
