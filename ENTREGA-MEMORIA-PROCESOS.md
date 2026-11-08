# Memoria de trabajo privada de procesos

Base: a0cf95e (v0.0.19). Integracion principal para v0.0.20.

## Contrato aprobado antes de implementar

Cada espacio x86 puede reservar hasta cuatro paginas de datos de 4096 bytes,
en 0x402000, 0x403000, 0x404000 y 0x405000. La reserva es bajo demanda;
las paginas fisicas son propias, se entregan en cero y sobreviven a ceder,
dormir y la preempcion. Codigo y pila conservan sus direcciones anteriores.

INT81/EAX18 reserva una pagina: retorna direccion virtual o cero al agotar
el cupo o la memoria. INT81/EAX19 libera EBX: retorna cero o -1 si es invalida.
Ambos servicios requieren un proceso gestionado; las rutas antiguas devuelven
-2 (servicio no soportado). La cabecera NXE2 y su ABI1 se conservan.

La API kernel valida ID completo y permite modificar mapas desde CR3 kernel
o el mismo espacio, nunca desde otro espacio privado. Las copias kernel validan
el rango completo antes de copiar; rechazan paginas no reservadas sin efectos
parciales. Activar reconstruye los mapas conservando las paginas de datos.

NEXTRY incorpora reservar_pagina(), liberar_pagina(direccion),
leer_entero(direccion) y escribir_entero direccion, valor. Las direcciones de
entero deben estar alineadas a cuatro y dentro de la zona de datos. Acceder a
una pagina liberada contiene un fallo de pagina en la aplicacion. La mascara
de permisos por operacion no cambia; el cupo limita el consumo de cada espacio.

Cancelar o terminar conserva las paginas para diagnostico hasta liberar el
proceso. Destruir el espacio devuelve las paginas adicionales y las cinco
paginas base. Los observadores FIN siguen protegiendo la destruccion.

Los buffers de los servicios de archivos y mensajes conservan los rangos
anteriores de codigo/pila: esta entrega no permite pasarlos desde los datos
dinamicos. No hay malloc de bytes, crecimiento automatico de pila, swap,
memoria compartida ni proteccion NX en este modo x86 sin PAE.
Una direccion antigua puede volver a ser valida al reservar de nuevo la misma
pagina virtual: no se incorporan identificadores de generacion para punteros.

## Archivos previstos y verificacion

- kernel/espacios.c y lib/espacios.h: mapas, reserva, liberacion y pruebas.
- kernel/excepciones.c: servicios 18 y 19 para el proceso actual.
- kernel/main.c y lib/nxk_es.h: diagnostico y puentes sin soporte explicito.
- lenguaje: compilador, runtime, ejemplo, editor y pruebas de memoria.
- Documentacion y version: registrar solo las comprobaciones ejecutadas.

Implementacion del gestor y servicios mediante puentes C; las aplicaciones se
escriben en NEXTRY. Una CPU, cuatro espacios, 16 KiB adicionales por espacio.
El gestor requiere metadatos y llamadores kernel confiables, fuera de IRQ.
Destruir devuelve las reservas una por una; no promete deshacer una liberacion
parcial si otro componente ring0 corrompio el asignador fisico.

## Pruebas ejecutadas en la integracion

- 64 pruebas de compilador:24 generales,12 IPC,12 dependencias,8 permisos y8 memoria.
- test.ps1 x86 -MemoryMatrix:32,64,128 y1024 MiB, arranque y pruebas internas
  superados. Se conserva el limite de128KiB; imagen normal73740B/test102412B.
- test.ps1 x64:64 MiB superado; imagen28680B. Las paginas privadas son solo x86.
- pruebas_memoria_procesos.py: cuota4/quinta0, paginas enteras en cero, doble
  liberacion, hueco/reutilizacion, registros y banderas, dos procesos en la
  misma VA, tres preempciones, cancelacion y devolucion exacta de paginas.
  Acceso tras liberar o sin reserva provoca vector14 sin detener el kernel.
  Fuente NEXTRY real termina42; casos de rango/alineacion y legacy rechazados.
- Regresiones permisos, resumen, dependencias, ejecucion privada y22 casos
  NXE2 desde disco superadas.
  Se comprueba memoria estable y discos de QEMU sin cambios usando snapshot.

El kernel de pruebas inicial excedio la reserva por el tamano de las pruebas
embebidas; se redujo ese helper y se mantuvo la cobertura extensa en la prueba
externa. La matriz se ejecuto con la version final que cabe en el limite.

## Usar el ejemplo

Compilar con `compilar-usuario.ps1 -Fuente lenguaje/ejemplos/pagina_memoria.nxf
-Salida build/x86/pagina.nxe`. Incluir pagina.nxe en un volumen NXFS como se
explica en lenguaje/FORMATO-NXE-USUARIO.md, montarlo y usar `proceso cargar
pagina.nxe`. `proceso ejecutar` completa el ejemplo con salida42.
`usuario listar` muestra cuantas paginas adicionales retiene cada espacio;
`proceso liberar ID` devuelve los recursos del proceso terminado.
