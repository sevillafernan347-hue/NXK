# Copia de memoria en NEXTRY ASM

El archivo `lib/copiar32.nxa` implementa memcpy para el kernel de 32 bits.
El ensamblador propio genera 22 bytes de codigo x86. `copiar_bytes` codifica
REP MOVSB: copia ECX bytes desde ESI hacia EDI. `direccion_adelante` limpia
el indicador de direccion. La rutina conserva ESI y EDI y devuelve el destino.
Las regiones de origen y destino deben ser validas y no solaparse.
No reemplaza a memmove ni comprueba permisos de memoria.

La construccion incorpora estos bytes al kernel. La variante de 64 bits
conserva una implementacion C. El compilador, el enlazador y buena parte
del kernel siguen usando herramientas externas: aun no hay autoalojamiento.

Las pruebas llaman a la rutina enlazada mediante un puntero volatile para
evitar que el compilador sustituya la llamada por su propia copia. Comprueban
longitud cero, varios tamanos, desalineacion, centinelas, origen y retorno.
Pasaron en QEMU x86 con 32/64/128/1024 MiB y x64 con 64 MiB.

Ejecutar: `powershell -ExecutionPolicy Bypass -File test.ps1 -Architecture x86 -MemoryMatrix`.
El codigo principal en NEXTRY sigue en `kernel/inicio.nxf`.
