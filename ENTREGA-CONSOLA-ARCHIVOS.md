# Configurar archivos de procesos desde consola

Base4c591ff, integracion aprobada para v0.0.23. Nuevo comando x86:
`proceso archivos ID nombre1,nombre2,nombre3,nombre4`.
Sin lista (`proceso archivos ID`) retira todo acceso a archivos.
El analizador limita ID decimal y cuatro nombres de23bytes, rechaza entradas
vacias, comas finales y exceso antes de invocar la API. La API conserva sus
reglas: solo subconjuntos, sin duplicados ni recuperacion de nombres, IDs
completos y procesos no terminales. No agrega syscall ni cambia ABI NXE2.
Puente C de consola; el filtro de NEXTRY sigue siendo el de v0.0.22.
Las pruebas cubren configuracion antes de ejecutar, subconjuntos, sintaxis
invalida sin efectos, terminales, IDs obsoletos y reciclaje.

Verificacion completada: matriz QEMU x86 con32/64/128/1024MiB y regresion
x64 con64MiB superadas. Prueba externa completa pruebas_archivos_procesos.py
superada, incluidos comandos invalidos sin efectos, subconjuntos, lista vacia,
terminales, generacion nueva y los casos FS/NEXTRY previos. Memoria y discos
estables. Imagen x86 normal73740B/test102412B; x64 normal24584B/test28680B.
