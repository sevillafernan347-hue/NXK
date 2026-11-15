# Validacion de estados ATA PIO

Base d8178e9 (v0.0.29). Integracion aprueba esta tarea para v0.0.30.
Alcance: kernel/ata.c, pruebas host del mismo fuente, documentacion y version.
Puente C; no cambiar ABI usuario, direcciones ni permiso de escribir solo
en esclavo. Mantener los cuatro bits altos de LBA28 y limites existentes.

Construir el controlador real con puertos sustituidos por funciones del
simulador, solo en un ejecutable de pruebas host. No ejecutar instrucciones
IN/OUT en Windows ni introducir simulacion en la imagen kernel normal.
Reproducir DRQ pendiente antes/despues de READ y tras IDENTIFY; el cierre
debe exigir BSY=0 y DRQ=0 antes de publicar datos/capacidad. Conservar error,
ausencia y timeout. Las salidas del llamador permanecen intactas al fallar.

Consultar protocolo PIO en borrador ATA/ATAPI-5 T13/1321D revision3,
distribuido por Seagate: https://www.seagate.com/support/disc/manuals/ata/d1153r17.pdf
Pruebas previstas: estados por fases, trazas de comandos/datos, limites,
ausencia, BSY/DRQ, ERR/DF, no escritura maestro y bits altos LBA28. Compilar
y arrancar x86/x64; verificar persistencia solo con discos virtuales propios.
La propuesta de Gemini no se copia automaticamente. Integracion aprueba tambien
documentar los errores en lib/ata.h e incluir la suite host en test.ps1 x86.

## Cambio implementado

READ espera BSY=0 y DRQ=0 antes del comando y tras las 256 palabras. IDENTIFY
espera ese mismo cierre antes de publicar modelo/capacidad; conserva -2 cuando
el dispositivo desaparece, -3 para ERR/DF y -4 para espera agotada. Antes
convertia todos los errores del cierre en -3 y aceptaba DRQ todavia activo.
Ambos caminos usan la pausa existente de cuatro lecturas de estado alternativo
despues de los datos. El destino se copia solo tras completar el cierre.

Referencia de protocolo: borrador T13/1321D rev3, seccion 7.15.6, bits BSY/DRQ.
Es una referencia de protocolo, no codigo de Linux ni certificacion ATA fisica.

El backend host se selecciona solo con NXK_ATA_PRUEBA_HOST en la unidad de
prueba. Incluye kernel/ata.c directamente y sustituye las cuatro primitivas
de puertos; no reimplementa la logica ni ejecuta IN/OUT en Windows. La imagen
normal mantiene las instrucciones originales y no incluye el simulador.

## Pruebas ejecutadas el 15 de noviembre de 2026

- Antes del arreglo: 46 comprobaciones, ocho fallidas. Reprodujeron aceptacion
  prematura de DRQ antes/despues de READ y despues de IDENTIFY, publicacion de
  capacidad indebida y perdida de errores de ausencia/espera agotada.
- Despues: `tools/python/python.exe lenguaje/pruebas_ata_estados.py` supera
  66 comprobaciones, compilado con -O2 -Wall -Wextra -Werror.
- La suite verifica READ/WRITE por fases, BSY transitorio/persistente, ausencia,
  DRQ pendiente o nunca presentado, ERR/DF, FLUSH, precedencia BSY sobre otros
  bits, exactamente 256 palabras y destino intacto ante cualquier fallo probado.
- Cuenta las cuatro lecturas de pausa tras datos; comprueba buffers desalineados,
  centinelas, limites LBA28, sus cuatro bits altos y maestro prohibido sin I/O.
  IDENTIFY fallido invalida capacidad y conserva la estructura destino completa.
- `test.ps1 -Architecture x86 -MemoryMatrix`: QEMU 32/64/128/1024 MiB, salida33
  y marcadores internos completos. `test.ps1 -Architecture x64`: 64 MiB.
- Tras integrar la suite en test.ps1 se ejecuto de nuevo x86 con 32 MiB:
  66 comprobaciones host y arranque completo superados desde el mismo comando.
- `lenguaje/pruebas_persistencia.py`: 38 resultados OK sobre discos virtuales
  descartables. Reinicios, ocho archivos, binario256 B, guardados cortados,
  corrupcion, recuperacion de banco anterior, generacion agotada, NXFS protegido
  y disco de arranque intacto. No se escribieron discos fisicos.

Archivo kernel x86 normal81932 B/pruebas114700 B; x64 normal24584 B/pruebas28680 B.
ABI, direcciones, sector512 B y limitacion de escritura a esclavo1 conservados.

## Limites

Puente C para x86, canal primario PIO/LBA28, un comando por vez. Sin IRQ ATA,
DMA, AHCI, concurrencia ni reset/recuperacion tras errores del canal. El limite
de sondeo sigue siendo un millon de lecturas, no un plazo medido por reloj.
Las pausas se cuentan pero no se mide su duracion fisica. Ni host ni QEMU
certifican el hardware ASUS. ERR/DF son errores generales; no se decodifica
ABRT por separado. Un fallo final de escritura no deshace los datos enviados;
la recuperacion logica de snapshots sigue correspondiendo a NXPD1.
