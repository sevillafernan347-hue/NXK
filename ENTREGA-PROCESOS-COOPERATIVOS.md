# NXK v0.0.13: procesos cooperativos reanudables

Desde v0.0.14 el PIT tambien suspende y reanuda procesos gestionados.
Consultar [la ampliacion de preempcion](ENTREGA-PREEMPCION.md); las referencias
al PIT terminal de este documento describen la entrega v0.0.13.

Base v0.0.12, commit3c6c491. Cuatro procesos sobre el pool de espacios privados.
NEXTRY ASM restaura CPU/IRET; C mantiene PCB, rondas y validacion del contexto.

## Uso

```text
proceso demo
volumen montar
proceso cargar proceso.nxe
proceso listar
proceso paso 257
proceso ejecutar
proceso liberar 257
```

257 es ejemplo: usar el ID devuelto por cargar. `proceso paso ID` ejecuta un
turno; `proceso ejecutar` recorre procesos listos por rondas hasta32turnos por
llamada. Si quedan listos, informa presupuesto agotado y se puede llamar otra
vez. Cargar no ejecuta. Terminar/fallar conserva recursos hasta liberar.
`proceso listar` muestra estado, turnos y cesiones, mas ultimo resultado o
error interno del motor.

NEXTRY ya admite `ceder` sin argumentos:

```text
funcion inicio
    entero valor = 23
    ceder
    afirmar valor == 23
    numero valor + 19
fin
```

`lenguaje/ejemplos/proceso.nxf` hace dos cesiones y muestra42. Compilar mediante
compilar-usuario.ps1 y empaquetar en NXFS como las demas aplicaciones NXE2.
La prueba `lenguaje/pruebas_procesos.py` deja un volumen de ejemplo en
build/x86/procesos-pruebas/procesos.img; conectar con run.ps1 -Volumen esa_ruta.

## Contexto conservado

El servicio opcional INT81/9 devuelve control al kernel; al continuar retorna
EAX0 a la aplicacion, que sigue despues de INT81. El wrapper guarda GS/FS/ES/DS,
los registros PUSHAD, EIP, CS, EFLAGS, ESPusuario y SS. El ESP auxiliar de PUSHAD
no se utiliza; POPAD lo omite. El ensamblador propio usuario_reanudar.nxa de26B
salva el retorno kernel, deshabilita IRQ durante la restauracion y vuelve por
IRET con el marco validado. Los bytes enlazados se verifican contra la fuente
NXA igual que en las otras entradas.

Cada PCB conserva marco CPU y espacio completo. Se valida EIP en codigo
0x400000..0x401000 y ESP en0x401000..0x402000 (extremo final permitido). CS43/SS51
son fijos. Selectores de datos se normalizan a16bits y solo admiten selectores
nulos o los segmentos usuario43/51. EFLAGS conserva solo CF/PF/AF/ZF/SF/DF/OF y
fuerza IF mas bit1; no restaura IOPL, TF, NT, VM ni otras banderas privilegiadas.
Un marco fuera de contrato se rechaza con error_motor6, antes de IRET.

No hay contexto FPU/MMX/SSE por proceso todavia. Durante esta ruta TS+MP enCR0
bloquean esas operaciones (#NM) y tambien FWAIT, y CR0 se restaura al volver.
Un uso invalido termina ese proceso; no se comparten registros FPU entre estos
procesos. El codigo NEXTRY actual usa enteros y no necesita FPU.

## Estado y API

Estados: LIBRE0, LISTO1, EJECUTANDO2, TERMINADO3, FALLIDO4. Una cesion vuelve a
LISTO; INT80 termina; excepcion o limite PIT termina como FALLIDO. Se conservan
turnos/cesiones saturados, ultimo resultado y codigo de error del motor si lo
hay. El pool no reserva heap propio; usa5paginas fisicas por espacio y la pila
kernel reutilizable del motor anterior.

lib/procesos.h ofrece crear/cargar, consultar/enumerar, paso, ejecutar con
presupuesto1..1024 y liberar. Rechazos no modifican las salidas. IDs completos
incluyen generacion del espacio: liberar por una API kernel externa retira el
PCB obsoleto en su siguiente consulta y nunca reutiliza su contexto sobre una
generacion nueva. `usuario turno ID` rechaza un ID asociado a proceso para no
reiniciar su pila mientras conserva EIP/registros. La demo rechaza procesos ya
registrados y limpia solo sus propias reservas al fallar por falta de slots.

API de una CPU, fuera de IRQ/tareas, CR3 kernel, PG/IF activas y motor inicializado.
No se reentra mientras organiza una ronda. Fallos CPU de una app no impiden que
la ronda ejecute otras. Un error interno negativo del motor detiene esa llamada
y deja el proceso FALLIDO; puede revisarse y continuar con los otros despues.
Una cesion de marco invalido puede haber entrado al historial como turno normal
antes del rechazo; para gestion de procesos prevalece estado/error_motor.

## Compatibilidad y limites

La cabecera NXE2 permanece enABI1; servicio9 es extension opcional de la ruta
cooperativa. Los servicios1..8 anteriores conservan comportamiento. El runtime
CPL3 de `ceder` exige retorno0; rutas anteriores sin reanudacion y NXE1 fallan
explicitamente. En codigo kernel `ceder` requiere tarea kernel actual.

El PIT sigue terminando bucles de usuario tras el limite existente: no guarda
el contexto para reanudacion automatica. La cesion es voluntaria. No hay SMP,
procesos esperando E/S, permisos de archivos ni contexto FPU. El FS RAM sigue
compartido. Sin NX en paging x86 noPAE; EIP reanudable se restringe al codigo.
Las pruebas son virtuales; la ASUS fisica sigue pendiente.

## Pruebas

Matriz x86 QEMU32/64/128/1024MiB y regresion x64 64MiB. Demo interna con dos
procesos y tres rondas: EBX/ESP independientes, dos cesiones y salida correcta;
cuatro slots, IDs obsoletos, error de una app sin bloquear otra y limpieza.
Prueba externa contrasta todos los GPR, CF/DF y ESP tras cesiones, NEXTRY con
locales, DS/ES/FS/GS nulos, FPU bloqueada, ESP0 rechazado sin caida, presupuesto
de32cesiones y reciclaje. Repetir no pierde paginas/guardas ni aumenta heap tras
calentar los buffers. Verifica discos intactos, ruta anterior sin servicio9,
NEXTRY legacy y tareas kernel. Compilador:23pruebas, incluido ceder y argumentos
invalidos. Regresiones de ejecucion privada y22NXE2 tambien superadas.
