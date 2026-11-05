# Cambios por version publicada

El detalle de todos los cambios permanece en el historial Git y los documentos
de entrega. Las etiquetas no reemplazan los commits intermedios.

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
