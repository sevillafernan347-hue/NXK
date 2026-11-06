# NXK v0.0.10: base de memoria privada para procesos

Base v0.0.9, commit314fa62. Puente C en kernel/espacios.c; paginacion x86
clasica, una CPU. Es un gestor de espacios de direcciones independiente del
runtime CPL3 y del planificador existentes.

## Capacidad implementada

Hasta cuatro espacios simultaneamente reservados. Cada uno posee cinco paginas
fisicas contiguas desde el asignador (20KiB): directorio, tabla baja, tabla alta,
codigo y pila. No consume heap. Cada espacio ve codigo en virtual0x400000 y
pila en0x401000, respaldados por paginas fisicas diferentes. El resto del mapa
kernel se mantiene supervisor. Codigo usuario solo lectura, pila lectura/escritura;
este modo x86 sin PAE no ofrece NX y por eso la pila no es no-ejecutable.

Los alias fisicos y las propias tablas siguen siendo supervisor. La pagina
nula continua ausente y los permisos de solo lectura del kernel se conservan.
Al activar se refrescan las tablas bajas desde el mapa kernel para recoger
guardas creadas despues de reservar el espacio. Para evitar anunciar guardas
que no esten en la copia activa, nxk_paginacion_guarda rechaza CR3 privado.
Debe llamarse desde el mapa kernel antes de activar el espacio.

`espacios` desde consola crea cuatro espacios, alterna CR3 tres veces por cada
uno, verifica codigo distinto y datos de pila independientes en las mismas VA,
rechaza destruir el activo, vuelve al directorio kernel y devuelve las20 paginas.
Al repetir no aumenta heap ni pierde paginas. Es una prueba desde ring0.

## API kernel

lib/espacios.h declara:

- `nxk_espacio_crear(codigo,longitud,&id)` admite1..4096 bytes y solo desde CR3
  kernel. Copia codigo en pagina inicialmente cero y genera ID distinto al
  reciclar slot; no reutiliza una generacion agotada. Salida intacta al fallar.
- `nxk_espacio_consultar(id,&info)` devuelve ID y direcciones fisicas del
  directorio, codigo y pila. Solo punteros de kernel confiables.
- `nxk_espacio_activar(id)` acepta CR3 kernel u otro espacio del pool y conmuta
  CR3, manteniendo el codigo kernel y sus pilas de supervisor accesibles.
- `nxk_espacio_restaurar()` retorna al directorio kernel0x200000.
- `nxk_espacio_destruir(id)` libera y borra las cinco paginas, solo desde el
  directorio kernel. Rechaza la destruccion del directorio activo.

Retornos:0 exito; -1 contrato/ID invalido; -2 pool lleno al crear o activo al
destruir; -3 contexto no permitido; -4 fallo de memoria fisica; -64 x64 no
implementado. API de una CPU, fuera de IRQ y tareas. Crear/destruir solo desde
kernel. Activar/restaurar no acepta CR3 ajeno. No ceder durante estas operaciones.

## Evidencias

Pruebas internas cubren cuatro reservas y rechazo del quinto, salida intacta,
20paginas consumidas/devueltas, codigo y pila en cero, permisos de PDE/PTE,
alias supervisor, IDs antiguos rechazados, reciclaje con nueva generacion,
alternancia real de CR3, rechazo de destruir activo y ausencia de fuga al repetir.
Una guarda inyectada despues de crear el espacio aparece al activarlo; se
restaura la PTE original al cerrar la prueba. Bajo CR3 privado se rechaza crear
una guarda en el mapa original. El test se ejecuta tras restaurar el mapa BIOS
real del asignador; no conserva reservas sobre sus pruebas de mapas sinteticos.

Matriz QEMU x86 32/64/128/1024MiB superada; regresion x64 64MiB superada.
La consola del planificador repite `espacios` y compara paginas, heap y guardas;
despues ejecuta NEXTRY. Consola general y22casos NXE2 comprueban que siguen
operativas las aplicaciones anteriores. Las pruebas virtuales no certifican ASUS.

## Limite de esta entrega

Esto no ejecuta procesos CPL3 concurrentes. El runtime anterior conserva su
contexto propio y no usa todavia este pool. Faltan conectar registros CPU,
TSS/pila de retorno, carga NXE, recursos por proceso, terminacion y planificador.
Los mapas kernel son compartidos supervisor y no constituyen aislamiento entre
componentes ring0. Antes de preempcion/SMP hacen falta exclusion e invalidacion
coordinada de mapas; las guardas nuevas se reflejan al volver a activar.

## Ampliacion v0.0.11: copias acotadas

`nxk_espacio_leer(id,virtual,destino,longitud)` copia codigo o pila al buffer
kernel. `nxk_espacio_escribir(id,virtual,origen,longitud)` acepta solo direcciones
de pila. Ambas exigen CR3 kernel e ID vigente antes de copiar; no activan otro
mapa. Validan longitud por resta para rechazar overflow y no copian parcialmente.
Retornos:0 exito, -1 ID/buffer, -2 rango, -3 contexto, -64 x64. Buffer nulo solo
con longitud0; virtual0x402000 solo con longitud0. Incluso n0 valida ID/contexto.
Los buffers kernel son confiables y no deben solaparse con tablas ni codigo.
La escritura rechaza direcciones virtuales de codigo; no es proteccion contra
un componente ring0 que escriba por un alias fisico. No son syscalls CPL3.

Pruebas: copia en pila sin modificar otro espacio, lectura cruzando codigo y
pila, rechazo de rangos fuera de paginas, overflow, null, longitud0, codigo RO,
contexto privado y salida intacta ante rechazo. Arranque x86 QEMU32MiB superado.
