# Preparacion de comandos ATA

Base 6ea7e69, v0.0.31. Rama kernel/ata-seleccion-v032. Integracion aprobada:
IDENTIFY espera reposo del dispositivo seleccionado antes de escribir parametros
y comando; en fase previa ERR aislado no bloquea una nueva solicitud explicita.
DF, ausencia, BSY/DRQ y errores durante/despues del comando se conservan.
Sin reset ni reintentos automaticos. No cambiar ABI, maestro protegido ni LBA28.

Referencia primaria: [borrador ATA/ATAPI-5 T13/1321D revision 3](https://www.seagate.com/support/disc/manuals/ata/d1153r17.pdf),
secciones 9.3 (figura 19) y 7.15.6.6 (ERR).

La espera anterior a escribir Device/Head no se incorpora en esta tarea: requiere
modelar por separado la unidad previamente seleccionada y la unidad destino.

## Comprobaciones

- Controlador anterior: 125 comprobaciones host, siete fallos reproducidos.
  IDENTIFY escribia parametros y comando con BSY, DRQ o dispositivo ausente.
  READ, WRITE y FLUSH rechazaban nuevas solicitudes con ERR residual aislado.
- Controlador corregido: 134 comprobaciones host sin fallos. Incluyen fallo
  previo sin escritura de parametros ni comando, invalidez de la capacidad
  hasta identificar de nuevo, BSY y DRQ transitorios, DF con ERR y los errores
  de la operacion en curso. El maestro sigue protegido y se conserva LBA28.
- Arranque QEMU x86 con 32, 64, 128 y 1024 MiB mediante
  `test.ps1 -Architecture x86 -MemoryMatrix`. Imagen normal 81932 B de kernel,
  imagen de pruebas 114700 B.
- `lenguaje/pruebas_ata_consola.py` sobre la imagen normal. QEMU registra el
  esclavo ausente con fase previa, emitido 0 y resultado -2. Las consultas
  repetidas conservan el diagnostico y la imagen original permanece intacta.
