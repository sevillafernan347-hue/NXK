# Lectura parcial de archivos para NEXTRY protegido

Nuevo servicio INT 0x81 numero 5: EBX nombre, ECX destino, EDX capacidad hasta
256, ESI desplazamiento uint32. EAX devuelve bytes, cero en EOF o FFFFFFFF por
error. El destino debe ser memoria escribible de usuario; cero bytes no escribe.
Desplazamiento mayor que longitud se rechaza antes de restar. El servicio 3
anterior de lectura completa conserva su significado y limite de capacidad.

La capa RAM/NXFS admite leer un intervalo: RAM tiene prioridad incluso vacia.
NXFS carga/verifica el archivo completo antes de copiar cada parte solicitada.
EOF y capacidad cero no publican bytes. Archivo vacio verifica su huella vacia.
Mostrar_archivo del runtime CPL3 repite solicitudes de 256 bytes hasta EOF;
puede mostrar archivos NXFS de hasta 4096 sin un buffer grande en la pila.
El puente kernel de NEXTRY tambien usa lectura por partes. La consola leer
conserva lectura completa de hasta 256; volumen leer admite hasta 4096.

Pruebas RAM: cola de dos bytes, EOF exacto, posicion posterior y UINT32_MAX.
Pruebas de servicio: desde 0/1/2, posicion 3 y UINT32_MAX en archivo de dos
bytes, destino codigo rechazado. Nueve escenarios NEXTRY/NXFS incluyen vacio,
1, 256, 257 y 4096 bytes; el maximo usa patron variable y se exige salida
completa en orden. Se conservan corrupcion, prioridad RAM y errores de montaje.
Matriz x86, consola general y regresion x64 superadas.

Limites: no descriptores abiertos ni snapshot de archivo por aplicacion.
Relee/verifica NXFS en cada transferencia; privilegia sencillez sobre rendimiento.
Sin concurrencia ni hotplug; si contenido cambia entre lecturas no se garantiza
una vista estable. Sin escritura al disco ni procesos multiples. El presupuesto
PIT y las restricciones actuales del runtime permanecen.
