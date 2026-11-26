# Auditoría defensiva de CPL3 y cargadores

Rama `kernel/seguridad-v044`, base `61db57d`. Alcance: entrada y retorno
de programas CPL3, servicios de archivos, cargadores NXE/PE32 y pruebas.
Se examinó el código de los mapas de usuario, los búferes y su entrega,
los permisos por proceso y la validación de ejecutables.

## Requisitos funcionales

- Usar la misma política de registros extendidos en toda transición a CPL3.
  Restaurar el control del procesador al volver al kernel.
- Mantener el código de usuario en la primera página de su mapa. Limitar
  el segmento de código hasta el final de esa página y conservar el mapa
  supervisor por debajo, las direcciones NXE2 y los selectores del ABI.
- Las lecturas completas de NXFS de procesos gestionados se difieren bajo
  el mismo mecanismo que los tramos. Una longitud mayor que el destino
  sigue siendo un error. Se conservan las reglas de nombre y permisos,
  el orden de búsqueda en RAM y los códigos del servicio.
- Rechazar entradas nulas en las interfaces de los cargadores de demostración.
  Los cargadores NXE1 y PE32 incorporados siguen destinados al binario propio.

La política de segmentos y CR0 sigue el [manual de programación del sistema
IA‑32 de Intel](https://cdrdv2-public.intel.com/671447/325384-sdm-vol-3abcd.pdf),
capítulos 2 y 5.

## Hallazgos y cambios

| Área | Hallazgo de la revisión | Corrección |
| --- | --- | --- |
| Registros de CPU | El bloqueo TS+MP de `kernel/excepciones.c` dependía de un contexto reanudable. Las rutas directas tampoco tienen un contexto propio de FPU. | Se aplica al cargar, entrar y reanudar CPL3. Se restaura CR0 en la salida recuperable. |
| Permisos de ejecución | El descriptor CS abarcaba todo el mapa lineal. Las páginas de pila y datos admitían escritura y quedaban dentro del segmento de código. | CS termina en el último byte de la página de código. Las páginas supervisadas inferiores siguen protegidas por paginación. |
| Disponibilidad | INT 81h difería los tramos de archivo. La lectura completa todavía podía esperar ATA dentro del servicio. | El proceso gestionado cede también para las lecturas completas de NXFS con datos. Un búfer insuficiente se rechaza antes de iniciar la lectura. |
| Cargadores de demostración | NXE1 y PE32 comprobaban el tamaño antes de acceder a la firma. Faltaba comprobar el puntero. | Ambos devuelven error para un puntero nulo. La consola sigue ejecutando el binario incorporado. |

## Alcance pendiente

El límite de segmento complementa la paginación x86 no PAE. NX por página
y las pruebas en hardware físico quedan pendientes. El puente PE32 y NXE1
de demostración se ejecutan con privilegios del kernel. El código NXE2
externo se ejecuta en CPL3. Las restricciones por nombre y permisos del
gestor se configuran con `proceso archivos` y `proceso permisos`; la ruta
`ejecutar usuario` conserva su acceso global a los servicios de archivo.
La operación de copiar archivos y las rutas directas conservan llamadas
síncronas. La validación FNV detecta corrupción accidental y no autentica
al autor de un ejecutable.

## Pruebas ejecutadas

- `test.ps1 -Architecture x86 -MemoryMatrix` pasó en QEMU con 32, 64,
  128 y 1024 MiB virtuales. La prueba del kernel leyó el límite del
  descriptor y comprobó que CR0 se restablecía al salir. La regresión
  x64 pasó con 64 MiB.
- `pruebas_procesos.py` comprobó el límite de CS desde CPL3 y la excepción
  por FPU en un proceso gestionado, un espacio privado directo y la ruta
  directa desde NXFS. La consola siguió operativa.
- Pasaron las regresiones de páginas de solo lectura y de búferes FS/IPC.
  `pruebas_lectura_diferida.py` verificó una lectura completa en espera,
  el rechazo de un destino corto sin modificarlo, la lectura por tramos,
  el cambio de montaje y la revocación del permiso.
- Pasaron los 22 escenarios de carga NXE2 desde disco, con validación de
  formato y conservación de la consola ante fallos de una aplicación.

La comprobación de CS mide el descriptor que ve el invitado en QEMU.
La validación de este mecanismo sobre la ASUS física sigue pendiente.
