# Diagnostico de operaciones ATA

Base d878e28, v0.0.30. Responsable de integracion aprueba API kernel aditiva
nxk_ata_consultar(unidad,salida) y comando de consola para v0.0.31.
Alcance: controlador/header ATA, consola, pruebas host/QEMU y documentacion.
Puente C, sin cambiar ABI NXE, permisos, direcciones ni formatos de disco.

Cada unidad conserva su ultima operacion valida: comando solicitado, LBA,
fase alcanzada, resultado, ultimo Status, registro Error y validez de este.
Consulta copia memoria sin tocar puertos; rechaza unidad/puntero invalidos
sin modificar salida. Inicialmente los registros son cero. Entradas invalidas
no borran diagnosticos anteriores. Una nueva operacion valida los reemplaza.
Error se lee solo con dispositivo presente, BSY=DRQ=0 y ERR=1; DF sin ERR no
autoriza interpretarlo. Un fallo antes de emitir comando puede pertenecer
a una operacion previa del disco: indicar comando_emitido por separado.

No recuperar ni reintentar escrituras automaticamente. Contar sondeos por
operacion, no tiempo real. Sin asignacion dinamica, IRQ ATA ni SMP. x64 devuelve
-64 sin modificar salida. Pruebas previstas: fases, registros, aislamiento entre
unidades, errores invalidos, consulta sin I/O y regresiones de arranque/persistencia.

## Uso implementado

En la consola, `discos` identifica las unidades; `discos estado` consulta lo
que quedo registrado. Es preferible consultar el estado inmediatamente despues
de un fallo de volumen/persistencia: otra operacion valida sustituye el registro.
No es un historial acumulado ni una lectura en vivo de los registros del disco.

Ejemplo observado en QEMU sin esclavo:

```text
IDE 0 comando 0x000000EC emitido 1 LBA 0 fase cierre resultado 0 estado 0x00000050 sondeos 2
IDE 1 comando 0x000000EC emitido 1 LBA 0 fase datos resultado -2 estado 0x00000000 sondeos 1
```

Comandos hexadecimales EC=IDENTIFY,20=READ,30=WRITE,E7=FLUSH. La fase indica
el ultimo punto alcanzado, incluso en exito. Sondeos suma lecturas de Status
en las esperas; no incluye pausas, datos o Error. No representa microsegundos.
Resultado -1 por argumento invalido no sustituye un diagnostico previo.
Resultados -2/-3/-4/-5 mantienen el significado documentado en lib/ata.h.

El registro consume72 B estaticos, sin heap. Las esperas antes duplicadas
comparten una rutina con igual politica de BSY/DRQ, ausencia y limite. Se agrega
una lectura de Error solo cuando su contenido es interpretable. Consultar
copia la estructura almacenada y no toca Status ni selecciona el dispositivo.

Referencia: [borrador ATA/ATAPI-5 T13/1321D rev3,7.11.3 y7.15.6](https://www.seagate.com/support/disc/manuals/ata/d1153r17.pdf).
Error requiere BSY/DRQ despejados y ERR activo. Los bits distintos de ABRT
dependen del comando: la consola etiqueta IDNF/UNC solo para READ emitido.
Los valores crudos permanecen disponibles para revisar otros errores.

## Evidencia del 15 de noviembre de 2026

- Suite host del controlador real:117 comprobaciones, cero fallos. Cubre
  estado inicial, consultas invalidas/sin I/O, registro por fase de READ/WRITE,
  ausencia, BSY/DRQ, ERR/DF, ABRT, sondeos, aislamiento de unidades, limpieza
  al iniciar otra operacion y conservacion ante argumentos rechazados.
- ERR con DRQ activo y otros bits con BSY activo no provocan lectura de Error.
  Se conservan pruebas de maestro prohibido, LBA alto, buffers y cierres previos.
- `test.ps1 -Architecture x86 -MemoryMatrix`:32/64/128/1024 MiB superados,
  salida33. El diagnostico de lectura tambien se valida dentro del kernel.
- `test.ps1 -Architecture x64`:64 MiB y nuevo marcador ATA SIN SOPORTE OK;
  API devuelve-64 sin tocar la estructura centinela.
- `lenguaje/pruebas_ata_consola.py`: arranque normal, estado inicial vacio,
  maestro identificado, esclavo ausente, resultado firmado y tres consultas
  repetidas identicas, ayuda actualizada y disco original intacto.
- `lenguaje/pruebas_persistencia.py`:38 resultados OK. Reinicios, snapshots
  cortados/corruptos, recuperacion del banco anterior, datos binarios, rechazo
  de formatos y generacion agotada; disco de arranque original intacto.

Imagen x86 normal81932 B/pruebas114700 B; x64 normal24584 B/pruebas28680 B.
Sigue siendo kernel experimental con puentes C. Hardware ASUS pendiente;
no hay nueva compatibilidad Windows, IRQ ATA, reset, DMA o recuperacion automatica.
