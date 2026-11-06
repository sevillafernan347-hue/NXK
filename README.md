# NXK y NEXTRY

Version publicada: **0.0.8**. Consulta [versiones e historial](VERSIONES.md) y
[los cambios por entrega](CAMBIOS.md).

Kernel original experimental para x86 de 32 bits, con objetivo de equipos de
1 GB de RAM. Incluye lenguaje NEXTRY, ensamblador NEXTRY ASM y formato NXE.

Empieza por [LEER-CODIGO.md](LEER-CODIGO.md), [ESTADO.md](ESTADO.md) y
[COLABORACION.md](COLABORACION.md). El codigo tiene comentarios en espanol.
Las propuestas de cambios se reciben mediante issues y pull requests; consulta
[CONTRIBUIR.md](CONTRIBUIR.md).

## Lenguajes utilizados

NEXTRY (`.nxf`), NXJ experimental (`.nxj`) y NEXTRY ASM (`.nxa`) son propios.
El kernel conserva puentes C; los compiladores usan Python y la construccion,
PowerShell y Batch. Consulta el [inventario de fuentes](LENGUAJES.md).
GitHub no admite inventar categorias nuevas mediante .gitattributes: NEXTRY
se documenta aqui; NEXTRY ASM se agrupa como Assembly.

## Aplicacion NEXTRY en modo usuario

`lenguaje/ejemplos/usuario.nxf` ya ejecuta texto, numeros y afirmaciones en CPL3.
En una nueva imagen, usa `usuario nextry`. Para integrar tu propia fuente:
`./build.ps1 -FuenteUsuario mi_programa.nxf`. En VS Code tambien existe la tarea
«NXK: Integrar aplicacion NEXTRY protegida actual» para el archivo abierto.
La app sigue integrada en la imagen, con codigo limitado a 4 KiB y runtime
inicial; consulta [la entrega](ENTREGA-NEXTRY-USUARIO.md).

## Aplicaciones NXE2 desde disco

Ya se puede compilar una fuente NEXTRY a NXE version 2 e incluirla en NXFS,
independientemente del kernel. Tras montar el volumen, usa
`ejecutar usuario disco.nxe`. Consulta [el formato y los pasos](lenguaje/FORMATO-NXE-USUARIO.md).
NXE version 1 conserva su ruta anterior en ring0; no se cambia silenciosamente.

## Copiar un archivo para editarlo

Con NXFS montado, `copiar leeme.txt editable` crea una copia en RAM de hasta
256 bytes. Puedes editarla con `agregar editable texto`. El destino debe ser
nuevo; no se reemplazan archivos existentes. Las copias permanecen en RAM; para conservarlas usa el guardado NXPD1 descrito abajo.
Consulta [la entrega](ENTREGA-COPIA-ARCHIVOS.md).
Desde una app NEXTRY usa `copiar_archivo "leeme.txt", "editable"`; hay un
[ejemplo completo](lenguaje/ejemplos/copiar_volumen.nxf) y
[pruebas del servicio protegido](ENTREGA-COPIA-NEXTRY.md).

## Tareas reutilizables del kernel

El planificador x86 admite ocho tareas cooperativas con argumentos, IDs que
cambian al reutilizar el slot, pilas con guarda y buzones de cuatro mensajes.
Desde la consola: `tareas preparar`, `tareas listar`, `tareas paso`,
`tareas ejecutar` y `tareas limpiar`. `tareas cancelar ID` cancela una tarea
lista o dormida; `tareas ocho` ejecuta la demostracion completa.
Consulta [el funcionamiento y sus limites](ENTREGA-PLANIFICADOR.md).
`tareas mensajes` demuestra espera sin consultas continuas y despertar por
envio o plazo; [detalle de la ampliacion](ENTREGA-ESPERA-MENSAJES.md).

`tareas resultados` demuestra dos tareas esperando el resultado de otra, con
plazo, rechazo de ciclos y proteccion frente a liberacion anticipada.
Consulta [la entrega](ENTREGA-RESULTADOS-TAREAS.md).

`tareas flujo` prueba envio y recepcion con plazo: un buzon lleno puede
bloquear al emisor hasta tener espacio. [Detalle y limites](ENTREGA-ENVIO-CON-PLAZO.md).

`tareas bloqueos` comprueba el rechazo de ciclos entre envio y espera de
finalizacion, incluidos anillos de ocho tareas. [Detalle](ENTREGA-CICLOS-CRUZADOS.md).

`tareas estadisticas` muestra los turnos, mensajes, esperas y plazos de cada
tarea registrada. Los contadores se conservan al terminar y empiezan en cero
al crear otra tarea. [Uso y significado](ENTREGA-METRICAS-TAREAS.md).

`tareas resumen` cuenta tareas pendientes/finalizadas por estado y muestra las
reservas de pila y mensajes en buzones. `version` informa el numero del kernel.

## Sincronizacion con semaforos

`semaforos` ejecuta ocho tareas compartiendo un recurso y verifica 32 operaciones
con cesiones dentro de la seccion protegida. La API kernel ofrece hasta ocho
semaforos contadores con plazo, IDs de generacion y destruccion protegida frente
a esperadores. Los diagnosticos incluyen esperas de semaforo.
Consulta [la entrega y sus limites](ENTREGA-SEMAFOROS.md).
La API tambien permite reservar/publicar lotes completos de permisos sin
reservas parciales; consulta [el contrato de lotes](ENTREGA-LOTES-SEMAFOROS.md).

El arranque NEXTRY ASM x86 admite imagenes de hasta 120 KiB en tres lecturas
sin cruzar fronteras fisicas de 64 KiB; imagen y variables quedan bajo 128 KiB.

## Mutex con propietario

`mutex` verifica ocho tareas y 32 operaciones protegidas. `mutex recuperar`
muestra como detectar un recurso abandonado y repararlo explicitamente antes
de reutilizarlo. La API rechaza liberacion ajena, recursion y ciclos entre
mutex y otras esperas; un recurso liberado sin reparar queda irrecuperable.
Consulta [contrato, pruebas y limites](ENTREGA-MUTEX.md).

## Guardar archivos entre reinicios

`persistencia guardar` captura los archivos RAM en un disco virtual NXPD1;
`persistencia cargar` los recupera y reemplaza la RAM actual. Dos bancos permiten
recuperar la copia anterior cuando la nueva esta incompleta. Hasta ocho archivos
de 256 bytes. Usa `run.ps1 -Persistencia build/x86/datos.img` con una imagen nueva
creada por `lenguaje/crear_persistencia.py`.
Consulta [pasos, formato y limites](ENTREGA-PERSISTENCIA.md) y
[lo que sigue pendiente](PENDIENTES-KERNEL.md).

## Deteccion de dispositivos y estabilidad

`pci` muestra los dispositivos de los buses PCI que el firmware configuro.
El kernel tambien contiene fallos de aplicaciones que llegan con segmentos
nulos y protege sus tablas de paginas ante un segundo inicio. Consulta
[la entrega y sus pruebas](ENTREGA-ESTABILIDAD-PCI.md).

## Construir en Windows

Las dependencias externas no se incluyen en Git. Consulta [HERRAMIENTAS.md](HERRAMIENTAS.md)
para versiones y origen: Zig en `tools/zig`, Python en `tools/python` y QEMU
en `tools/qemu`. La laptop original ya las tiene. No copies credenciales al repositorio.

```powershell
powershell -ExecutionPolicy Bypass -File build.ps1 -Architecture x86
powershell -ExecutionPolicy Bypass -File test.ps1 -Architecture x86 -MemoryMatrix
powershell -ExecutionPolicy Bypass -File run.ps1 -Architecture x86
```

El mapa BIOS E820 se recoge en NEXTRY ASM y se consulta mediante un puente C.
La consola informa RAM descrita por BIOS y espacio del heap por separado.
El gestor x86 ya reserva y libera paginas fisicas de 4 KiB hasta 1 GiB.
Cada pagina se pone en cero antes de entregarla a un nuevo usuario.
El mapa BIOS se analiza uniendo regiones de RAM y respetando reservas solapadas;
los archivos RAM se pueden recortar o ampliar con ceros desde la API kernel.
Consulta [la entrega de robustez](ENTREGA-ROBUSTEZ-MEMORIA.md).
Ya hay paginacion inicial, contexto CPL3 reutilizable y archivos RAM. Faltan
procesos CPL3 simultaneos, un sistema de archivos general e aislamiento completo.
El compilador genera C; el kernel aun no esta escrito completamente en NEXTRY.
El EXE de demostracion usa solo tres funciones Win32: no hay compatibilidad general.

`Biblioteca`, herramientas y archivos generados se conservan localmente y no
forman parte de este repositorio. No se ha definido una licencia de redistribucion
del codigo propio. Las dependencias conservan sus licencias originales.
