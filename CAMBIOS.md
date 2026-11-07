# Cambios por version publicada

El detalle de todos los cambios permanece en el historial Git y los documentos
de entrega. Las etiquetas no reemplazan los commits intermedios.

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
