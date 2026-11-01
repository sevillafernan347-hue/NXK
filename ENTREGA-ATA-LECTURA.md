# Controlador ATA PIO de solo lectura

Codigo original en kernel/ata.c y lib/ata.h. Canal IDE primario, unidades
maestro/esclavo, IDENTIFY DEVICE y READ SECTORS LBA28 de un sector. Capacidad
limitada a 2^28 sectores de 512 bytes (128 GiB). Modelo ATA convertido de palabras
con bytes intercambiados, caracteres no imprimibles saneados. ATAPI no soportado.
Sondeo de BSY/DRQ/ERR/DF limitado a un millon de lecturas de estado, no plazo
real garantizado. IRQ de dispositivo deshabilitada, sin DMA ni concurrencia.

Lectura exige identificacion, unidad valida, LBA dentro de capacidad y destino
kernel no nulo de 512 bytes. Lee a buffer temporal y copia solo tras exito.
No hay comando ATA de escritura en esta implementacion. No conecta aun el
sistema de archivos RAM al disco y no ofrece sectores al usuario CPL3.

Comando discos: modelo y cantidad de sectores de las dos unidades. Pruebas de
QEMU esperan imagen primaria de 1 MiB, 2048 sectores, esclavo ausente: lectura
antes de identificar rechazada, firma 55AA del sector 0, sector 1 igual al codigo
kernel cargado, ultimo sector cero, limites/unidades/destino invalidos sin cambiar
buffer, centinelas y lectura maestro despues de sondear esclavo ausente.
Matriz x86 32/64/128/1024 MiB, consola discos/help y regresion x64 superadas.

Solo probado con IDE emulado de QEMU. No certifica ASUS ni SATA AHCI/NVMe,
USB o discos fisicos. Variante x64 devuelve no disponible para este controlador.
Puente C con operaciones de puertos, pendiente migracion NEXTRY/NEXTRY ASM.
Siguiente etapa de almacenamiento: interfaz de bloques y formato persistente
probado exclusivamente sobre una imagen virtual separada.
