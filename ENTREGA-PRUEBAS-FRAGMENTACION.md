# Pruebas de espacios con paginas fisicas aisladas

Base bfc8c8b. Subtarea de pruebas aprobada por integracion para v0.0.29.
Archivo previsto: kernel/pruebas_espacios_fragmentados.c. La integracion
agrega el enlace y la llamada desde la imagen NXK_TEST; no cambia la API.

Funcion nxk_espacios_fragmentados_probar(), sin heap y con almacenamiento
acotado en pila. Solo agota la RAM en configuraciones de hasta 8192 paginas
libres. Exige CR3 kernel y slots vacios. Conserva una lista de paginas retenidas
en sus propios bytes, libera huecos alternos y comprueba rollback con 0..4
paginas libres, creacion con cinco aisladas, permisos y traducciones reales,
copias entre codigo/pila, activacion, reuso y recuperacion del diagnostico.

El helper restaura CR3 y libera sus reservas al terminar, tambien ante un
fallo de asercion. No inicializa un mapa sintetico ni toca paginas ajenas.
No ejecuta codigo CPL3; esa regresion corresponde a la prueba de integracion.
Integracion compilo y supero la matriz QEMU x86 32/64/128/1024 MiB y x64
64 MiB el 14 de noviembre de 2026. El agotamiento especial se ejecuto en32 MiB;
las configuraciones mayores mantienen las pruebas generales del gestor.
