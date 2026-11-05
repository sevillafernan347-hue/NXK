# NXPD1: archivos que sobreviven al reinicio

Entrega experimental v0.0.7 sobre v0.0.6 (ccd3335). Implementacion original;
puentes C en kernel/persistencia.c, kernel/ata.c y kernel/archivos.c.

## Usar en la maquina virtual

Desde la carpeta del proyecto, crear una imagen nueva una sola vez:

```powershell
tools/python/python.exe lenguaje/crear_persistencia.py build/x86/datos.img
./run.ps1 -Architecture x86 -Persistencia build/x86/datos.img
```

El generador usa apertura exclusiva y rechaza reemplazar archivos existentes.
La imagen mide 9728 bytes (19 sectores). `run.ps1` comprueba archivo, extension,
tamano y firma; el kernel valida el formato entero antes de escribir.
El disco de arranque se abre con snapshot; solamente este archivo de datos
recibe escrituras persistentes. Las pruebas usan archivos virtuales descartables.

En NXK:

```text
escribir nota Hola desde NXK
persistencia guardar
```

Cerrar la maquina virtual y abrirla con el mismo comando. En NXK:

```text
persistencia cargar
leer nota
```

Guardar captura los ocho slots RAM (hasta 256 bytes por archivo, nombres de
hasta 23 caracteres); cargar REEMPLAZA todos los archivos RAM por esa captura.
Un guardado vacio tambien es valido. Guardar/cargar son explicitos: modificar
un archivo en RAM no lo guarda automaticamente. No hay montaje NXPD1 automatico.
`-Volumen` NXFS y `-Persistencia` son excluyentes porque ambos usan el esclavo IDE.
El lector NXFS existente sigue siendo de solo lectura y se rechaza para guardar.

## Formato y confirmacion

Enteros little endian, sectores de 512 bytes:

| Sector | Contenido |
| --- | --- |
| 0 | Superbloque NXPD0001 |
| 1 | Cabecera banco A |
| 2..9 | Ocho registros banco A |
| 10 | Cabecera banco B |
| 11..18 | Ocho registros banco B |

Superbloque: firma ASCII NXPD0001 en 0..7; version1 en8; sectores19 en12;
entradas8 en16; bytes/sector512 en20; ceros24..507; FNV1a32 en508 sobre0..507.
Se exige capacidad ATA exactamente19, no se exploran particiones ni se formatea.

Cabecera de banco: firma NXPS en0..3; version1 en4; generacion no cero en8;
cantidad0..8 en12; FNV1a de los4096 bytes de datos en16; ceros20..507;
FNV1a de la propia cabecera0..507 en508.
Cada registro512: nombre24 (NUL y relleno0), longitud32bits en24, ceros28..31,
datos256 en32..287 (relleno0 desde longitud), ceros288..511. Slot vacio: todo0.
Se rechazan duplicados, nombres invalidos, longitudes excesivas y relleno sucio.

Antes de guardar se leen y verifican ambos bancos. Se elige el valido de mayor
generacion; generaciones iguales validas se rechazan por ambiguedad. Se escriben
los ocho sectores del banco inactivo, se ejecuta FLUSH CACHE, se escribe su
cabecera como confirmacion y se vuelve a ejecutar FLUSH CACHE. Finalmente se
relee y verifica. La generacion UINT32_MAX impide nuevos guardados, sin wrap.
El banco anterior permanece disponible mientras se prepara el siguiente.

Una lectura ATA fallida aborta toda la operacion; no se trata como checksum
incorrecto. Un banco corrupto puede descartarse si el otro es valido. Si ambos
estan totalmente en cero se permite el primer guardado. Si no hay ninguno
valido y contienen datos, se rechaza: la primera escritura interrumpida puede
requerir recrear un disco virtual nuevo. No se borra o repara silenciosamente.

Cargar valida disco y contenido antes de importar. Importar prepara y valida
los ocho archivos antes de reservar la tabla o modificarla; un error conserva
la RAM anterior. La tabla se reutiliza y una importacion vacia inicial no la crea.

## API y errores

`nxk_persistencia_guardar()` / `nxk_persistencia_cargar()`:

| Retorno | Significado |
| --- | --- |
| 0 | Exito |
| 1 | Sin instantanea al cargar; RAM conservada |
| -1 | Disco ausente/no identificable |
| -2 | Formato o capacidad incorrectos |
| -3 | Sin banco valido o generaciones ambiguas |
| -4 | Error E/S o verificacion final |
| -5 | Generacion agotada |
| -6 | Error de exportacion/importacion RAM |
| -7 | Invocacion desde una tarea |
| -64 | Arquitectura no implementada |

`nxk_archivos_exportar/importar` usan arrays de ocho `nxk_archivo_imagen`, sin
solaparlos con la tabla interna. API kernel exterior a tareas, fuera de IRQ,
una CPU y sin reentrada. No se agrega syscall NXE ni palabra NEXTRY.

## Evidencia y limites

Matriz x86 QEMU32/64/128/1024MiB; regresion x64; pruebas RAMFS integradas con
importaciones validas/invalidas y heap estable. Pruebas de persistencia incluyen
reinicios reales de QEMU, ocho archivos, vacio, binario256, 18 interrupciones
simuladas de datos (segundo y tercer guardado), cuatro cabeceras parciales,
corrupcion de cabecera/datos/nombre/longitud/padding/duplicados, disco sin formato,
NXFS ajeno, generaciones ambiguas y agotadas, y primer guardado incompleto.
Se comprueba disco de arranque intacto y rechazo/carga sin escrituras extra.

Los cortes se simulan mezclando sectores de imagenes conocidas: no se ha
inyectado un fallo electrico real ni un error del dispositivo ATA. FNV detecta
corrupcion accidental, no autentica. Un error despues de enviar la confirmacion
puede dejar resultado incierto; releer determina que copia es valida. No hay
garantia contra un dispositivo que incumpla FLUSH o pierda ambos bancos.

Es persistencia inicial de instantaneas pequenas. Faltan directorios, archivos
grandes, permisos, almacenamiento concurrente y controladores modernos. No se
ha instalado en disco fisico ni certificado la ASUS. No es un sistema terminado.

Referencias del protocolo ATA (no codigo reutilizado):
- https://www.seagate.com/support/disc/manuals/ata/d1153r17.pdf
- https://wwwaem.seagate.com/content/dam/seagate/migrated-assets/www-content/product-content/skyhawk/en-us/docs/100870481a.pdf
