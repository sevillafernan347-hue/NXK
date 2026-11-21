# Auditoria de procedencia y cambios independientes

Revision de la historia disponible hasta NXK 0.0.35. La auditoria se centra en
fuentes versionadas, recursos y tablas con indicios concretos de procedencia
externa, con atencion especial a MINI NXK y al teclado. No es una certificacion
legal ni demuestra originalidad absoluta. La historia de Git permite seguir
cambios, pero no reemplaza registros de diseño o licencias de terceros.

## Hallazgos

| Archivo y fragmento | Fuente/comparacion | Clasificacion y decision |
| --- | --- | --- |
| `kernel/interfaz.c`, `letras[36][7]` del commit `980f61a` (v0.0.27), por ejemplo digito cero `{14,17,19,21,25,17,14}` y digito dos `{14,17,1,2,4,8,31}`; casos de `bits()` | Se encontraron esos diez patrones de digitos en una tabla de fuente 5x7 de un proyecto Wokwi y filas de letras/cifras coincidentes en `BlockGlyph()` de otro proyecto publico. Las referencias estan abajo. El commit NXK agrega el archivo de una vez, lo llama “propia”, pero no conserva bocetos, autor ni procedencia. No se pudo verificar derivacion, cronologia de origen o licencia aplicable a NXK. | **Coincidencia exacta de datos; origen indeterminado, no prueba de copia.** Por no poder acreditar la procedencia, se reemplazaron los glifos alfanumericos por matrices 3x5 nuevas, centradas en la celda 5x7. No se trasladaron las tablas encontradas. La puntuacion geometrica se conserva; no se encontro una coincidencia concreta de esa parte. |
| `kernel/main.c`, arreglo `keys[]` de `read_char()`, por ejemplo indices `0x02` a `0x0b` traducidos a `1` a `0` y `0x1e` traducido a `a` | Ya aparece en el commit inicial `bc11d03` (31-oct-2026), sin fuente citada. Los valores corresponden al mapa de posiciones PC/AT de codigos Set 1; IBM documenta la correspondencia teclas/codigos y OSDev publica la tabla Set 1 para US QWERTY. | **Coincidencia normativa/de hardware; no se encontro evidencia de copia de una implementacion.** La correspondencia de codigos no se debe cambiar para aparentar originalidad. Se conserva y documenta su alcance: distribucion US, sin Shift ni teclas extendidas/acentos. La tabla traduce codigos observados por la ruta PC/AT; no afirma que todos los teclados PS/2 transmitan nativamente Set 1. |

## Especificacion independiente de los glifos nuevos

Antes de escribir las matrices se fijaron estos requisitos: celda de 5x7,
alfabeto ASCII en mayusculas/minusculas sin distincion, cifras, contraste de un
pixel, cinco filas activas de hasta tres columnas, centrado horizontal y una
fila vacia arriba y abajo. Las formas se dibujaron manualmente como matrices
pequenas para mantener separaciones legibles en 320x200; `bits()` coloca esas
matrices en el centro de la celda. No se trasladaron las coordenadas de la
tabla anterior. El historial conserva la sustitucion como diff revisable.

Los glifos son una composicion nueva para NXK, no una afirmacion de exclusividad
frente a toda forma tipografica posible. La puntuacion existente conserva sus
reglas geometricas sencillas. La prueba grafica debe comprobar legibilidad en
la salida renderizada, no solo que el codigo compile.

## Revision del historial y del arbol

- La fuente investigada de `kernel/interfaz.c` entra como archivo nuevo en
  `980f61a`; no hay commits previos de esa tabla para reconstruir su creacion.
- La tabla `keys[]` se encuentra en el commit inicial del proyecto y no tiene
  una nota de atribucion propia. La similitud con las tablas publicadas se
  explica por la codificacion estandar de teclas, no permite inferir copia.
- Se buscaron archivos rastreados de fuentes tipograficas, imagenes, ROMs,
  carpetas de proveedores y avisos de copyright/licencia en fuentes. No se
  encontro un paquete de fuente o recurso grafico externo rastreado en Git.
- Los dos commits de merge visibles se revisaron: `4e279ba` actualiza
  documentacion propia y `68eb18d` integra el PR #1, limitado a instrucciones
  de contribucion/aportes. No aparece una importacion/merge de un kernel. Esto
  no descarta fragmentos derivados sin marcas o material que nunca estuvo en
  Git; una auditoria automatica de similitud tampoco prueba autoria.
- El repositorio no declara licencia para el codigo NXK. La autoria registrada
  y esta auditoria no definen por si mismas condiciones de redistribucion. Las
  dependencias locales excluidas de Git tienen sus propias licencias pendientes
  de verificar antes de distribuir herramientas o binarios.

## Fuentes consultadas

- IBM, *Personal Computer AT Technical Reference*, septiembre de 1985,
  seccion “Keyboard Scan-Code Outputs” y tablas de teclas/codigos, documento
  historico: <https://www.bitsavers.org/pdf/ibm/pc/at/6139362_PC_AT_Technical_Reference_Sep85.pdf>.
- OSDev Wiki, “PS/2 Keyboard”, tabla de scan codes Set 1 para teclado US
  QWERTY: <https://wiki.osdev.org/PS2_Keyboard>.
- Proyecto Wokwi, `font[11][7]`: los elementos 0..9 coinciden con las diez
  matrices de cifras investigadas. La pagina no acredita que su autor sea
  fuente original de esos patrones:
  <https://wokwi.com/projects/480463961880365057>.
- Proyecto `aesthetic.computer/core`, `xbox/native-bios/App.cpp`, funcion
  `BlockGlyph()`: otra tabla publica contiene coincidencias exactas con varias
  letras y cifras. Tampoco se establece aqui que sea la fuente de NXK ni su
  licencia aplicable:
  <https://tangled.org/aesthetic.computer/core/blob/main/xbox/native-bios/App.cpp>.
- Historial local de NXK: commits `bc11d03` (mapa inicial) y `980f61a`
  (incorporacion de MINI NXK); referencias y decisiones se registran aqui.
- `HERRAMIENTAS.md` y `CONTRIBUIR.md`: inventario de dependencias y declaracion
  actual de que el codigo propio no tiene licencia de redistribucion definida.

La documentacion de IBM sustenta que los codigos son una correspondencia de
hardware. La tabla secundaria OSDev se uso para contrastar la secuencia Set 1;
no se copio codigo de esa pagina. Las coincidencias bitmap encontradas permiten
describir el solapamiento concreto, no atribuir una direccion de copia. Como no
se pudo verificar la creacion o licencia de la tabla antigua, se reemplazo con
matrices nuevas y se conserva la evidencia en el historial de esta rama.

## Pruebas y limites de verificacion

Resultados de esta revision: compilaciones x86 normal y MINI NXK correctas;
`experimentos/mini-so/probar.py` superada, con captura PNG inspeccionada
visualmente; `test.ps1 -Architecture x86 -MemoryMatrix` superada en QEMU con
32/64/128/1024 MiB, incluidas las 141 comprobaciones ATA host y los 16 tramos
NXFS; regresion `test.ps1 -Architecture x64` superada con 64 MiB. La prueba
grafica comprueba arranque, color, teclado PS/2, comandos, archivos, NEXTRY,
procesos, retroceso, scroll y discos intactos. QEMU no certifica la ASUS fisica
ni distribuciones de teclado distintas.

La revision cubre los candidatos concretos encontrados en el arbol y el
historial Git disponible, no una comparacion exhaustiva de cada funcion contra
todo el software publico. No se identifico una implementacion externa que
podamos concluir que fue copiada. La licencia general del proyecto y la
procedencia de dependencias locales siguen pendientes.
