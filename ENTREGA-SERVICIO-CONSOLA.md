# Primer servicio retornable de usuario: consola

INT 0x81, servicio EAX=1: EBX apunta a bytes y ECX indica cantidad (0..256).
EAX devuelve bytes escritos, FFFFFFFF para direccion/longitud invalidas o
FFFFFFFE para servicio desconocido. Cero bytes no accede al puntero. Se valida
CPL3 y ejecucion activa, longitud y rango completo dentro de codigo/pila de
usuario. Se copia a un buffer kernel antes de escribir; no interpreta formatos.
No es una API estable para aplicaciones externas todavia.

Puerta temporal con DPL3, entrada NEXTRY ASM guarda registros y segmentos,
normaliza DS/ES y pasa el contexto al puente C. Modificar EAX guardado entrega
el resultado al usuario. IRETD restaura el contexto; INT 0x80 sigue siendo salida.
La puerta INT 0x81 anterior se restaura al terminar o recuperar un fallo.
La validacion actual usa las dos paginas conocidas, no un gestor general de VM.

Comando usuario texto: programa conocido escribe texto, verifica resultado y
termina con 42. Casos negativos: puntero al kernel, 257 bytes, servicio desconocido,
intervalo cerca de UINT32_MAX y cruce fuera de las dos paginas. Caso vacio verifica
cero bytes con puntero no valido sin lectura. Todo se repite sin consumo extra
tras inicializar. Matriz x86, consola y regresion x64 superadas.

Cargador BIOS x86 lee 96 sectores (48 KiB), manteniendo destino 0x10000 y firma
512 bytes. Transferencia menor a 64 KiB y 127 sectores. Imagen de pruebas de
32776 bytes verificada. Kernel total con BSS limitado a 64 KiB de RAM, reservada
por el gestor fisico. Variante x64 conserva lectura y limite de 32 KiB.

Limites: copia solo desde dos paginas fijas de demostracion. Sin concurrencia,
permisos por archivo o procesos multiples. Escritura serie sincronica y hasta
256 bytes; un dispositivo sin respuesta puede bloquear al kernel. No garantiza
plazos de tiempo real. El servicio C es puente pendiente de migracion a NEXTRY.
