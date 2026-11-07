# NXE version 2: aplicaciones de usuario x86

Formato original propio; conserva extension .nxe y magic NXE seguido de cero.
Version 1 usa API de punteros y ejecucion ring0. Version 2 usa servicios de
interrupcion y ejecucion CPL3. Los cargadores se distinguen explicitamente;
no se convierte una app antigua a usuario ni se acepta PE/EXE como NXE2.

Cabecera fija de 64 bytes, little-endian:

| Offset | Tipo | Valor |
| --- | --- | --- |
| 0 | 4 bytes | NXE seguido de cero |
| 4 | uint16 | Version 2 |
| 6 | uint16 | Arquitectura 1 (x86 32 bits) |
| 8 | uint32 | Cabecera 64 |
| 12 | uint32 | Base virtual 0x400000 |
| 16 | uint32 | Entrada relativa 0 |
| 20 | uint32 | Bytes de codigo/constantes: 1..4032 |
| 24 | uint32 | Memoria virtual 8192: codigo y pila |
| 28 | uint32 | FNV1a de bytes posteriores a cabecera |
| 32 | uint32 | ABI usuario 1 (INT 0x80 salida, INT 0x81 servicios) |
| 36 | 28 bytes | Reservados en cero |

Longitud de archivo exactamente 64 + payload, hasta 4096 para NXFS. Codigo y
constantes en pagina de solo lectura; pila en pagina escribible. No datos
globales, relocalizaciones, importaciones ni entry alternativo. FNV detecta
corrupcion, no autentica ni firma. El linker y empaquetador comprueban contrato.
Ejecucion correcta de NEXTRY finaliza con 0; fallo de runtime con 1. El 42 sigue
siendo marcador exclusivo de demostraciones de puente y resultado del ejemplo.

Compilar con salida propia: compilar-usuario.ps1 -Fuente mi_programa.nxf
-Salida build/x86/disco.nxe (una sola linea). Los intermedios de salida propia
no reemplazan el binario integrado al kernel. Crear imagen nueva:
tools/python/python.exe lenguaje/crear_volumen.py build/x86/mis-apps.img
--archivo disco.nxe=build/x86/disco.nxe (una sola linea). Iniciar run.ps1
-Volumen build/x86/mis-apps.img; comandos volumen montar y ejecutar usuario disco.nxe.
No es necesario reconstruir el kernel para cada aplicacion si ya tiene NXE2.

Solo lee desde NXFS montado; no ejecuta una copia RAM con el mismo nombre.
Valida antes de copiar/cambiar CR3. Un buffer de carga se reutiliza, con contexto
privado de usuario. Tras terminar, fallar o agotar diez ticks se restaura kernel.
No hay multiples procesos ni planificador entre apps; no es sandbox certificado
para codigo desconocido. Apps comparten archivos RAM sin ACL. Las escrituras a
disco aun no existen. Fallos sincronicos CPL3 se contienen, excepto NMI, doble
fallo y machine-check; fallos kernel permanecen fatales.

Pruebas: empaquetador con validacion de limite/ELF/cabecera; dieciseis imagenes NXFS
con app correcta, campos invalidos, checksum/truncamiento, bucle infinito y
BOUND fuera de rango. Se verifica que formatos rechazados no produzcan historial
de ejecucion, que consola siga operativa y que imagenes base no cambien.
Los diez escenarios de excepciones anteriores y matriz x86/x64 se conservan.

La entrada NEXTRY ASM limpia EAX/EBX/ECX/EDX/ESI/EDI/EBP antes de IRET;
ESP conserva la pila usuario. Una imagen raw verifica registros iniciales en
cero. Se mide heap antes/despues de cargas repetidas y un rechazo intermedio.

El ejemplo usuario_disco.nxf lee tambien leeme.txt del mismo volumen. Las
pruebas comparan heap antes/despues de tres cargas con rechazo intermedio y
verifican que compilar con -Salida conserve el binario integrado. VS Code
incluye tarea NXK: Compilar aplicacion NXE2 actual. La imagen de demostracion
generada por pruebas queda en build/x86/nxe-disco-pruebas/correcto.img, con
disco.nxe y leeme.txt; se puede conectar mediante run.ps1 -Volumen esa_ruta.

El ejemplo de disco tambien copia leeme.txt a RAM, agrega un marcador y borra
la copia al finalizar. Tres recargas comprueban que la orden copiar_archivo
funciona desde un NXE2 independiente sin conservar destinos ni consumir heap.

## Extension cooperativa opcional desde v0.0.13

INT81/EAX9 cede solo en la ruta `proceso`. El kernel conserva contexto y al
reanudar devuelve EAX0. El runtime NEXTRY expone `ceder`; la cabecera sigueABI1.
Rutas sin continuacion devuelven servicio no soportado y el runtime falla
explicitamente. Ver ENTREGA-PROCESOS-COOPERATIVOS.md para permisos y limites.

Desde v0.0.15 INT81/EAX10, EBX=ticks espera en procesos y retorna0 al despertar.
Cero equivale a ceder; maximo7fffffff. La ABI1 se mantiene. El plazo es interno
al kernel; no modifica EBX. Ver ENTREGA-DORMIR-PROCESOS.md.

Desde v0.0.16 hay servicios opcionales de mensajes para procesos gestionados:
INT81/EAX11 envia (EBXdestino, ECXvalor); EAX12 recibe (EBXbuffer8, EDXticks);
EAX13 consulta el ID actual. Recepcion0=mensaje,1=vacio/plazo, negativos=error.
Buffer solo pila privada y plazo0..7fffffff, validados antes de consumir datos.
ABI1 y cabecera sin cambios; legacy rechaza estas extensiones.
Ver ENTREGA-MENSAJES-PROCESOS.md y ejemplos proceso_mensajes/proceso_receptor.
