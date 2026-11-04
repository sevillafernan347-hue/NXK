# Cambios por version publicada

El detalle de todos los cambios permanece en el historial Git y los documentos
de entrega. Las etiquetas no reemplazan los commits intermedios.

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
