# Una lectura verificada para mostrar archivos NXFS

Base 3455a49, v0.0.32. Integracion aprueba la API interna aditiva
`nxk_volumen_visitar` para v0.0.33. Alcance: lector NXFS, sus dos
llamadores de salida, pruebas y documentacion. Se preservan formato NXFS1,
ABI NXE, API de lectura por rangos y prioridad de archivos RAM.

El visitante es sincrono y solo recibe bytes despues de validar el archivo
completo. El buffer pertenece a NXFS y solo vive para la llamada. El visitante
no puede volver a llamar al volumen, ceder ni retener el puntero.

`volumen leer` imprime desde el buffer ya verificado. Al leer el primer
archivo no vacio se reservan 4096 bytes del heap, frente a 8192 en la consola
anterior. Un archivo ausente no causa esa reserva y las lecturas siguientes
reutilizan el buffer. La ruta de NEXTRY dentro del kernel lee y verifica
una vez en lugar de hacerlo por cada tramo de 256 bytes. RAM sigue teniendo
prioridad sobre NXFS para ese servicio de NEXTRY.

ATA coloca cada sector directamente en su tramo del buffer. Una longitud de
hasta 4096 garantiza que el ultimo sector cabe. La huella abarca exactamente
la longitud del archivo. Ante fallo de lectura o huella, el visitante nunca
recibe datos y la lectura por rangos deja intacto el destino del llamador.
Los rangos vacios conservan su comportamiento anterior.

Validacion en QEMU: matriz x86 con 32, 64, 128 y 1024 MiB y x64 con 64 MiB.
Pasaron las 134 comprobaciones ATA y las 38 verificaciones de persistencia.
`lenguaje/pruebas_volumen.py` supero 18 escenarios. Comprobo dos lecturas de
4096 bytes exactos con el mismo buffer, ausencia sin reservar y rechazo sin
imprimir contenido cuando se altera el ultimo byte del archivo.

La prueba adicional de NEXTRY en modo usuario se detuvo en el caso de
archivo de 4096 bytes. El mismo caso se reprodujo con v0.0.32, antes del
cambio. El archivo de 4096 bytes si se mostro completo desde la consola.
La ruta de modo usuario se corrigio en v0.0.34. Ver
ENTREGA-LECTURA-SECUENCIAL.md.

Puente C. Un lector por vez en contexto kernel cooperativo. La verificacion
usa FNV1a para detectar corrupcion accidental. La ASUS fisica sigue pendiente.
