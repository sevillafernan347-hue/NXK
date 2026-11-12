# Cambios por version publicada

El detalle de todos los cambios permanece en el historial Git y los documentos
de entrega. Las etiquetas no reemplazan los commits intermedios.

## v0.0.22

- Lista de hasta cuatro nombres de archivo por proceso. Sin filtro al crear;
  despues de activarlo solo se aceptan subconjuntos. Lista vacia irreversible.
- Servicio INT81/20 y API kernel validan tabla completa, nombres y duplicados
  antes de modificar la politica. Nombres exactos y copias internas canonicas.
- FS2..8 exige permisos generales y filtro por nombre. Copiar/renombrar exige
  ambos nombres antes de consultar o modificar archivos; denegacion devuelve-5.
- NEXTRY limitar_archivos con0..4 textos, ejemplo comentado y editor. Cancelar
  conserva el filtro; una generacion nueva comienza sin restricciones.
- proceso listar muestra si el filtro esta activo y cuantos nombres conserva.
- 80 pruebas compilador, matriz QEMU x86 32/64/128/1024MiB y regresion x64
  superadas. Prueba externa de limites, huecos, subconjuntos, permisos, FS2..8,
  aislamiento, IDs y ejemplo real superada; memoria/discos estables.
- Regresiones de buffers, permisos, dependencias, resumen y22 escenarios
  NXE2 desde disco superadas.
- Registro temporal de rondas en pila de la prueba libera BSS sin eliminar
  aserciones ni cambiar las pilas de tareas; limite de arranque conservado.
- Puentes C; filtro por nombres en FS compartido, sin usuarios/propietarios ni
  ACL por objeto. Detalle: ENTREGA-ARCHIVOS-PROCESOS.md.

## v0.0.21

- Servicios de salida, archivos y buffers IPC en paginas de datos privadas.
  Validacion completa de rangos, cruces y respaldos antes de copiar o consumir.
- Retencion de paginas de recepcion/FIN pendiente: liberar individualmente
  devuelve-5, incluso tras vencer el plazo hasta despachar. Cancelar/consumir
  retira la retencion; destruir el receptor sigue recuperando sus recursos.
- Cinco expresiones NEXTRY para leer/guardar archivos, mostrar bytes, recibir
  mensajes y esperar resultados en memoria. Estados manejables, dos ejemplos,
  editor y ocho pruebas nuevas de aridad/tipos.
- 72 pruebas de compilador, matriz QEMU x86 32/64/128/1024MiB y x64 64MiB
  superadas. Prueba externa de FS1..8, cruces/huecos, permisos, esperas,
  plazos, resultados y limpieza sin efectos parciales ni cambios en discos.
- Regresiones de memoria dinamica, permisos, dependencias y22 escenarios
  NXE2 desde disco superadas.
- Minimizacion de funciones de prueba y kernel_main x64 conserva los limites
  BIOS sin retirar aserciones. README normalizado a UTF-8.
- Experimental, cuatro procesos, una CPU y puentes C; no agrega E/S asincrona
  de dispositivos ni mensajes de bloques. Ver ENTREGA-BUFFERS-PROCESOS.md.

## v0.0.20

- Hasta cuatro paginas privadas de datos por espacio, reservadas bajo demanda
  y puestas en cero. Las mismas direcciones virtuales usan respaldos distintos.
- INT81/18 reserva y19 libera para el proceso actual. Cupo4, direcciones
  exactas y TLB actualizados; rutas antiguas sin PCB rechazan las extensiones.
- Las paginas sobreviven a cesion/preempcion y se recuperan al liberar el
  proceso, incluso tras fallo o cancelacion. Terminar conserva el diagnostico.
- Copias kernel entre paginas fisicas no contiguas con validacion completa:
  un hueco rechaza todo el rango sin copiar parcialmente.
- NEXTRY reservar_pagina(), liberar_pagina(), leer_entero() y escribir_entero;
  ejemplo comentado, editor y validacion de tipos/alineacion/rangos.
- 64 pruebas compilador; matriz QEMU x86 32/64/128/1024 MiB y regresion x64
  superadas. Aislamiento, limpieza, cupos, fallos contenidos y recursos estables.
- Regresiones de permisos, resumen, dependencias, ejecucion privada y22 casos
  NXE2 desde disco superadas; discos originales intactos.
- Puentes C; datos limitados a16KiB, sin malloc por bytes ni NX. Los buffers
  de archivos/mensajes siguen en codigo/pila. Detalle: ENTREGA-MEMORIA-PROCESOS.md.

## v0.0.19

- Cuatro permisos por proceso: salida, leer archivos, modificarlos y enviar
  mensajes. Defaults15 conserva apps anteriores; solo se pueden quitar bits.
- Consulta y reduccion en INT81/16..17, consola proceso permisos ID MASCARA.
  Mascara0 e idempotencia validas; escalacion, bits desconocidos e IDs viejos
  rechazados. Cancelar conserva mascara; nueva generacion empieza15.
- Servicios de archivos/salida comprueban permiso antes de copiar o modificar;
  copiar exige lectura+modificacion. IPC inmediato y pendiente valida permiso.
- Revocar un envio suspendido lo despierta sin entregar ni formar ciclos falsos.
- NEXTRY restringir_permisos y permisos_actuales(), ejemplo real y editor.
- 56 pruebas compilador; matriz x86 32/64/128/1024 MiB y regresion x64 superadas.
  Pruebas de denegacion sin efectos, subconjuntos, revocacion, IDs y memoria.
- Regresiones resumen, dependencias, IPC y22NXE2 superadas; discos intactos.
- Permisos por operacion, sin usuarios/ACL por archivo ni proteccion de rutas
  antiguas sin PCB. Detalle: ENTREGA-PERMISOS-PROCESOS.md.

## v0.0.18

- API y comando proceso resumen: estados almacenados, pendientes/finalizados,
  mensajes en cola y observadores de resultados. Sin asignar heap ni depurar PCB.
- Omite IDs obsoletos; errores conservan salida. Libre no garantiza un espacio
  disponible, porque el pool tambien admite espacios sin PCB.
- Matriz x86 32/64/128/1024 MiB y regresion x64 superadas. Pruebas de estados
  mixtos, referencias, cola terminal, consultas repetidas y plazos sin despertar.
- Regresion completa de dependencias/NEXTRY superada; recursos y discos estables.
- Detalle: ENTREGA-RESUMEN-PROCESOS.md.

## v0.0.17

- Envio CPL3 con espera de espacio, plazo original y valor retenido en el PCB.
- Observacion de resultados con varios esperadores, copia completa del registro
  y retencion del ID protegida tambien frente a la API antigua de espacios.
- Estados8/9, referencias, contadores y cancelacion de las nuevas esperas.
- Detector acotado de ciclos FIN/ENVIO, incluidos los creados por rellenar una
  cola despues de despertar. Rebloqueo sin turno CPU extra si se pierde el hueco.
- NEXTRY enviar_esperando, esperar_proceso y finalizar codigo. Tres ejemplos
  comentados, editor y comando proceso dependencias con pruebas de arranque.
- 48 pruebas del compilador y matriz QEMU x86 32/64/128/1024 MiB superadas;
  regresion x64 64 MiB y demostracion repetida con recursos estables.
- Prueba externa: FIFO/backpressure, plazos, registros/banderas, limites,
  referencias, fallos/cancelacion, IDs reciclados y ciclos de dos/tres procesos.
  Tres ejemplos reales juntos verifican12mensajes, suma78 y salidas42/42/0.
- Regresiones de IPC, procesos, ejecucion privada, dormir, preempcion, consola,
  tareas y22casos NXE2 superadas. Discos virtuales originales intactos.
- Limites y pasos de uso: ENTREGA-DEPENDENCIAS-PROCESOS.md.

## v0.0.16

- FIFO4 de mensajes por proceso, valor uint32 y remitente real con ID completo.
- Recepcion no bloqueante o con plazo, estado6, copia completa antes de consumir.
- Cancelacion estado7 de listos/dormidos/esperadores; ID retenido hasta liberar.
- INT81/11..13 opcionales y NEXTRY enviar_mensaje/recibir_mensaje/proceso_actual().
- Contadores, comandos consola, dos ejemplos y soporte del editor actualizado.
- 36 pruebas del compilador; QEMU verifica FIFO/full, origen entre dos procesos,
  punteros/plazos invalidos, GPR/CF/DF, vencimiento, cancelacion e IDs antiguos.
- Matriz x86 32/64/128/1024 MiB y regresion x64 64 MiB superadas. Regresiones
  de dormir, procesos, preempcion y 22 casos NXE2 superadas; memoria estable
  y discos virtuales originales intactos. No equivale a ausencia de errores.
- Detalle y limites: ENTREGA-MENSAJES-PROCESOS.md. El historial se conserva.

## v0.0.15

- Estado DORMIDO, plazos modulares y retorno inmediato si todos esperan.
- Servicio opcional INT81/10 conserva registros y calcula plazo al pedir dormir.
- Orden NEXTRY dormir expresion, cero como cesion y limites validados.
- Demo/pruebas de reanudacion, otra app avanzando, maximo, errores y reciclaje.
- Matriz x86, regresion x64 y24pruebas del compilador superadas.

## v0.0.14

- El PIT suspende y permite reanudar procesos CPL3 al agotar10 ticks por turno.
- EAX original y marco completo conservados; motivo quantum distinto de ceder.
- Contador de preempciones, rondas acotadas y demo proceso preempcion.
- Prueba de GPR/ESP/DF/FS/GS, progreso de otra app y memoria estable.
- Ejemplo NEXTRY sin ceder; tests de bucle sin bloqueo, segmentos nulos y legacy.
- Matriz x86, regresion x64, procesos, ejecucion privada, consola/tareas y22NXE2.

## v0.0.13

- Cuatro PCB cooperativos con registros, EIP/ESP y segmentos conservados al ceder.
- Nuevo retorno CPL3 en NEXTRY ASM, con validacion de marco y EFLAGS seguros.
- Rondas con presupuesto, estados, diagnosticos, IDs generacionales y liberacion.
- Comandos proceso y orden NEXTRY ceder mediante INT81/9 opcional.
- Contexto FPU aun no soportado: bloqueado durante los procesos con TS+MP.
- Pruebas de GPR/CF/DF/ESP, segmentos nulos, marco invalido, FPU y NEXTRY.
- Matriz x86, regresion x64,23pruebas compilador, ejecucion privada y22NXE2.

## v0.0.12

- Ejecucion CPL3 sincronica en espacios retenidos, con pila propia conservada.
- Carga NXE2 validada sin ejecutar; comparte validacion con el cargador anterior.
- Comandos usuario cargar/listar/turno/liberar y demo usuario espacios.
- Restaura CR3, TSS y puertas de interrupcion tras salida, fallo o limite PIT.
- Pruebas de cuatro apps, datos separados, escritura RO, bucle, EAX alto y limpieza.
- Matriz x86, regresion x64, prueba externa de turnos, consola/planificador
  y22casos NXE2 superados.

## v0.0.11

- Copias acotadas entre buffers kernel y espacios privados: lectura codigo/pila,
  escritura solo pila, rechazo sin copia parcial y comprobaciones sin overflow.
- Pruebas de aislamiento entre espacios, fronteras y contexto; x86 32MiB superado.

## v0.0.10

- Pool x86 de cuatro espacios privados con IDs de generacion y cinco paginas cada uno.
- Codigo RO y pila RW en las mismas VA con respaldo fisico independiente.
- Alternancia de CR3, retorno al mapa kernel y liberacion de paginas; comando espacios.
- Refresco de guardas al activar y rechazo de modificar guardas desde CR3 privado.
- Matriz x86, regresion x64, consola/planificador y 22 NXE2 superados.
- Gestor de memoria preparatorio; procesos CPL3 simultaneos siguen pendientes.

## v0.0.9

- Busqueda de dispositivos PCI por clase con salida intacta si no hay coincidencia.
- Comandos `pci red` y `pci almacenamiento`; pruebas de limites y contraste QMP.
- Arranque x86 de 32 MiB y dos topologias PCI verificados.

## v0.0.8

- NEXTRY ASM normaliza segmentos al entrar por excepcion y antes de salida INT80.
- Paginacion acepta mapa RAM contiguo fragmentado y rechaza reinicializacion.
- Inventario PCI de solo lectura, con funcion multiple, puentes y limite seguro.
- Seis nuevos casos NXE2 de segmentos nulos y pruebas PCI contra QMP.
- Matriz x86 32/64/128/1024 MiB, regresion x64 64 MiB, 22 NXE2,
  consola, planificador y persistencia superados.

## v0.0.7

- Guardar y recuperar archivos RAM entre reinicios mediante instantaneas NXPD1.
- Dos bancos con generacion/checksums y confirmacion despues de sincronizar datos.
- ATA PIO escritura/FLUSH restringidos al esclavo identificado; arranque protegido.
- Importacion RAM valida toda la imagen antes de publicar, con relleno canonico.
- Generador de disco nuevo y opcion run.ps1 -Persistencia; comandos en consola.
- Pruebas de reinicio, cortes simulados, corrupcion, vacio, binario y limites.
- Guia de uso/formato y listado honesto de capacidades pendientes.
- Matriz x86, regresion x64, consola, nueve NXFS y dieciseis NXE2 superados.

## v0.0.6

- El gestor une regiones de RAM contiguas o solapadas antes de identificar
  paginas completas; conserva prioridad de reservas y valida los metadatos.
- La memoria descrita por BIOS evita duplicar solapamientos y descuenta reservas.
- Nueva API kernel para recortar y ampliar archivos RAM, con relleno de ceros.
- Pruebas de mapas fragmentados/desordenados y limites de archivos en RAM.
- Matriz x86, regresion x64, consola y 16 escenarios NXE2 superados.

## v0.0.5

- Adquirir/publicar varios permisos de semaforo como una operacion indivisible.
- Espera con plazo original, cancelacion sin consumo parcial y suma sin overflow.
- API anterior conserva comportamiento delegando en lotes de una unidad.
- Cinco grupos nuevos de pruebas: contratos, despertares insuficientes, plazo,
  cancelacion y limites uint32; heap/guardas estables.
- Matriz x86 32/64/128/1024 MiB, regresion x64 64 MiB y consola del planificador
  superadas. Sin cambios de ABI NXE ni nuevos objetos dinamicos.

## v0.0.4

- Ocho mutex con propietario, IDs de generacion, plazos y destruccion protegida.
- Deteccion de abandono al terminar/cancelar propietarios; recuperacion explicita
  y estado irrecuperable si se libera sin reparar.
- Detector de ciclos ampliado a propietarios de mutex, envio y finalizacion.
- Comandos mutex y mutex recuperar, con nuevo estado y contador de diagnostico.
- Modulo kernel/mutex.c con pruebas de exclusion, propiedad, cancelacion y ciclos.
- Matriz x86, regresion x64, seis controles de carga, consola, 16 NXE2
  y diez escenarios de excepciones/privilegios superados.

## v0.0.3

- Ocho semaforos contadores kernel: crear, consultar, adquirir con plazo,
  publicar sin desbordar y destruir con proteccion de esperadores.
- IDs con generacion, cancelacion de esperas y nuevo estado/contador de diagnostico.
- Comando semaforos: ocho tareas y 32 operaciones sobre un recurso compartido.
- Arranque NEXTRY ASM por bloques de 32/32/56 KiB: capacidad de imagen 120 KiB,
  reserva total 128 KiB, unidad BIOS preservada entre transferencias.
- Prueba de carga completa con cinco marcadores y corrupciones controladas.
- Matriz x86, regresion x64, consola, 16 NXE2, diez excepciones/privilegios
  y siete pruebas del ensamblador superados.

## v0.0.2

- Numero del kernel unificado en lib/version.h, mostrado al arrancar y con version.
- Consulta global de tareas registradas, pendientes y finalizadas, con desglose
  por estados, reservas de pila y cantidad de mensajes retenidos en buzones.
- Nuevo comando tareas resumen; consulta sin asignar memoria ni alterar tareas.
- Metadatos invalidos se rechazan sin usar estados como indices de escritura.
- Inicio de etiquetas anotadas y notas publicadas para conservar entregas.
- Pruebas x86 32/64/128/1024 MiB, regresion x64, consola y 16 escenarios NXE2.

## v0.0.1 — snapshot 358156c

Primera etiqueta de referencia, asignada a la entrega anterior ya probada.
Incluye todo el trabajo acumulado hasta ese commit:

- Kernel propio x86 BIOS, memoria, paginacion, excepciones y temporizador.
- Aplicaciones NEXTRY protegidas y carga NXE2 desde NXFS de solo lectura.
- Archivos RAM, lectura parcial, copias, agregado y renombrado.
- Ocho tareas cooperativas, buzones, esperas con plazo y resultados compartidos.
- Rechazo de ciclos de dependencia y estadisticas por tarea.

Limites comunes: sistema experimental con puentes C, una CPU y planificador
cooperativo kernel. Sin aplicaciones Windows generales, disco escribible ni
procesos CPL3 simultaneos. Las pruebas virtuales no certifican hardware ASUS.
