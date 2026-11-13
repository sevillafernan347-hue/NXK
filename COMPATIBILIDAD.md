# Objetivo de compatibilidad: laptops con 1 GB de RAM

NXK debe mantenerse util en equipos con 1 GB de RAM instalada. Este requisito se refiere al sistema que construiremos, incluidas las aplicaciones basicas; el editor y el compilador de desarrollo tienen sus propios requisitos.

## Lo que se puede verificar ahora

La variante principal del prototipo arranca en una PC virtual BIOS x86 de 32 bits con CPU `qemu32`. `PROBAR-RAM.cmd` o `PROBAR-NXK-32.cmd` compila y repite la prueba de arranque con 32, 64, 128 y 1024 MiB de RAM virtual. Guarda los resultados en `build/x86/memory-test-results.json`. La cantidad reservada por QEMU no representa el consumo real del kernel.

Resultado del 31 de octubre de 2026: la variante de 32 bits supero las cuatro configuraciones, incluido el modulo del lenguaje, la aplicacion NXE y el EXE Win32 de demostracion. El tamano actual del kernel se informa al compilar; no representa su uso total de RAM. La variante x64 tambien supero una prueba de regresion con 64 MiB y la aplicacion NXE.

Estado v0.0.26: x86 usa segmentos planos y paginacion supervisor por identidad
hasta1GiB, mapa BIOS E820 y asignador de paginas desde8MiB. Cuatro procesos
CPL3 tienen memoria privada, datos RO/RW, cuotas, preempcion y esperas. El heap
de laboratorio sigue siendo512KiB incremental; no equivale a toda la memoria
disponible. La variante x64 conserva su arranque inicial y mapa de2MiB, sin
las capacidades de procesos x86. Consulta [mapa de memoria](MAPA-MEMORIA.md).

La entrega actual supero QEMU x86 con32/64/128/1024MiB y x64 con64MiB. Ademas
del arranque se prueban asignacion, procesos y servicios de x86; no es una
medicion completa de respuesta, autonomia ni compatibilidad fisica.

El cargador NXE reserva su region de laboratorio en `0x180000` (hasta 128 KiB). El puente Win32 de x86 usa `0x400000` (hasta 128 KiB). Son regiones fijas para estas pruebas, todavia no asignaciones gestionadas a partir del mapa de memoria real de la ASUS.

## Presupuestos de diseno

Estos son objetivos para las siguientes etapas, no resultados medidos del sistema completo:

- Kernel y servicios basicos: hasta 64 MiB de memoria residente.
- Entorno de consola util: hasta 128 MiB en total.
- Futuro escritorio ligero y aplicaciones basicas: hasta 512 MiB en reposo.
- Mantener margen para las aplicaciones y la memoria que reserve el firmware o el hardware.
- Evitar servicios obligatorios que requieran red o mucha memoria para arrancar.

Mediremos cada etapa y revisaremos su presupuesto antes de anadir mas funciones.

## ASUS fisica: pendiente de identificar

El usuario indica que la ASUS es de 32 bits, de alrededor de 2010 y con 1 GB de RAM. Ese es ahora el objetivo principal. Necesitamos modelo exacto, CPU y tipo de arranque para establecer compatibilidad real.

- La variante principal x86 ya implementa el arranque de 32 bits y exige BIOS con lectura extendida del disco. No intenta entrar en modo largo ni ejecutar instrucciones de 64 bits.
- Si el equipo requiere UEFI, necesitaremos un cargador UEFI adecuado.
- Teclado USB, almacenamiento, pantalla y red pueden necesitar controladores que NXK aun no tiene.

No se ha probado NXK en la ASUS ni se ha preparado un instalador para su disco. Primero identificaremos el equipo y validaremos un medio de prueba externo.

## Proxima implementacion

Los proximos hitos incluyen espera de dispositivos, controladores y pruebas
fisicas, ademas de ampliar recursos de procesos. El [trabajo pendiente](PENDIENTES-KERNEL.md)
distingue estas capacidades del prototipo ya probado.
