# NXK v0.0.12: programas cargados en espacios privados

Base v0.0.11, commit e97f9a8. El pool de memoria se conecta con la entrada CPL3
NEXTRY ASM y los manejadores existentes. Motor y demostraciones son puentes C.

## Uso desde consola

`usuario espacios` demuestra cuatro programas cargados a la vez: dos contadores
separados, un intento de escritura en codigo RO y un bucle detenido por PIT.
Tras los fallos se vuelve a ejecutar un programa valido y se devuelven paginas.

Para usar aplicaciones NXE2 de un volumen NXFS:

```text
volumen montar
usuario cargar disco.nxe
usuario listar
usuario turno 257
usuario liberar 257
```

El ID 257 es solo un ejemplo: usar el numero que devuelve cargar/listar. Se
admiten hasta cuatro espacios retenidos. Cargar valida todo NXE2 antes de
reservar paginas y copia el payload a su propia pagina; reutilizar el buffer
del siguiente archivo no cambia programas cargados. Un ID de un slot liberado
queda invalido, aunque se vuelva a ocupar ese slot. El quinto se rechaza.
Los comandos previos `ejecutar usuario nombre.nxe` y `usuario nextry` conservan
su ruta sincronica anterior y siguen funcionando.

Cada turno entra por0x400000 con ESP0x402000 y registros limpios. Los bytes de
pila0x401000..0x402000 se conservan entre turnos, por eso una aplicacion puede
mantener datos propios alli. El turno empieza otra vez desde la entrada: no
continua el EIP/ESP ni los registros del turno anterior. Salir, fallar o agotar
el tiempo no destruye el espacio; liberar es explicito.

## Contratos del motor

`nxk_usuario_ejecutar_espacio(id,&resultado)` usa solo un contexto CPU a la vez,
exige paginacion activa, CR3 kernel original, IF habilitada, GDT/TSS y PIT
iniciados, y ausencia de tarea kernel actual. Reutiliza una pila kernel de4KiB
reservada en el primer turno valido. Durante la ejecucion instala puertas
INT80/81 y TSS temporalmente; despues restaura CR3, puertas y TSS antes de
publicar la salida. Crear guardas sigue requiriendo el directorio kernel.

Retorno0 significa resultado disponible; `resultado.fallo` diferencia salida
normal de excepcion/PIT. EAX puede ser cualquier uint32, incluso0xffffffff o
0x80000020, sin que eso solo signifique fallo. El resultado alimenta tambien
el historial de ocho ejecuciones existente. Los errores previos conservan la
salida y no publican historial:

| Retorno | Significado |
| --- | --- |
| -1 | ID, salida nula o motor ya activo |
| -2 | Contexto no permitido o activacion rechazada |
| -3 | Sin heap para primera pila kernel |
| -4 | Plantilla de servicio NXA no valida |
| -5 | Retorno CR3/CS inesperado o fallo de restauracion |
| -64 | Arquitectura no implementada |

`nxk_usuario_crear_espacio_nxe(datos,n,&id)` comparte con el cargador anterior
la validacion de firma, version, arquitectura, base, cabecera, tamano, ABI,
reservados y checksum. Luego delega la reserva en el pool. Formato/checksum
incorrectos no crean espacios ni modifican ID. `nxk_usuario_preparar_archivo`
usa NXFS y buffers kernel de lectura reutilizados, nunca un buffer4KiB local.
El llamador libera con `nxk_espacio_destruir` despues de usar el ID.
`nxk_espacio_enumerar` permite listar slots sin exponer punteros internos; sus
errores conservan la salida.

## Verificacion

Pruebas de arranque: cuatro espacios CPL3 vivos, A=1/B=10/A=2, fallo de escritura
RO (#PF14,error7), bucle limitado por PIT(vector32), A=3 despues de los fallos,
bytes separados e intactos y retorno exacto de paginas/guardas. Repetir no crece
heap tras la primera reserva. Tambien IDliberado/null, IFoff y CR3privado con
salida intacta, y EAX0xffffffff normal. Las reservas se liberan antes de ejecutar
los mapas sinteticos del asignador fisico.

Prueba externa `lenguaje/pruebas_ejecucion_privada.py`: carga cuatro NXE2,
contrasta listado, rechaza quinto y demo sin slots sin alterar paginas, conserva
contadores al alternar, captura fallo/bucle, rechaza IDviejo/overflow y formato
invalido, recicla slots, salida normal0x80000020 y NEXTRY retenido ejecutado dos
veces. Despues verifica heap/paginas y ejecuta NEXTRY legacy y tareas. Discos
originales protegidos con snapshot y comparados por bytes al finalizar.

## Limites

Es ejecucion secuencial protegida de programas retenidos. Todavia faltan PCB,
registros CPU conservados, espera/reanudacion, planificacion y recursos por
proceso. El PIT termina un bucle, no lo pausa para reanudar. Las llamadas kernel
no son interrumpidas por ese limite CPL3. Codigo solo lectura, pila escribible;
no hay NX en paging x86 no PAE. Los alias/tablas fisicos son supervisor.

Los servicios INT81 existentes y archivos RAM son comunes: no se incorporaron
permisos de archivos por programa. Sin SMP, fuera de IRQ y tareas, sin prueba
fisica ASUS. Las pruebas de QEMU no equivalen a certificacion del hardware.
