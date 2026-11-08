# NXK v0.0.18: resumen de procesos sin modificar su estado

Base v0.0.17, commit397024b. La sesion principal aprueba la nueva API kernel
nxk_procesos_resumen y el comando `proceso resumen`; no cambia NXE ni syscalls.
Puente C de diagnostico, junto al gestor de procesos existente.

El comando muestra registrados, pendientes, finalizados, mensajes acumulados
en las colas y referencias de resultados pendientes. Desglosa los diez estados.
Los finalizados incluyen TERMINADO, FALLIDO y CANCELADO; todos los demas
procesos registrados cuentan como pendientes.

La instantanea usa los estados almacenados: no despierta plazos, no ejecuta
procesos ni limpia PCB. Un DORMIDO cuyo plazo vencio sigue apareciendo asi
hasta que listar/consultar/planificar refresque su estado. Las lecturas repetidas
no cambian turnos, contadores, memoria ni historial.

LIBRE significa sin proceso registrado vivo; no garantiza capacidad para crear.
`usuario cargar` puede reservar un espacio del mismo pool sin crear un PCB.
Se omiten PCB cuyo ID completo ya no tiene respaldo; un ID reciclado nunca se
cuenta como la generacion anterior. `referencias_fin` cuenta observadores, no
destinos unicos, e incluye observadores LISTO con una entrega aun pendiente.
Los mensajes que conserva un proceso terminal cuentan hasta su liberacion.

La API no asigna heap y publica la estructura completa despues de validar.
Rechaza null(-1), metadatos invalidos(-2) y contexto no permitido(-3), sin
modificar la salida. Requiere CR3 kernel, IF habilitada y estar fuera de tareas
y del ejecutor CPL3. La variante x64 conserva el rechazo -64 de este gestor x86.

Pruebas internas comprueban cuatro procesos listos y rechazo sin salida parcial.
La prueba externa `lenguaje/pruebas_resumen_procesos.py` verifica estados,
colas, referencias, IDs obsoletos, consulta repetida y ausencia de despertares.
Los resultados ejecutados se registran en CAMBIOS.md y ESTADO.md al publicar.
