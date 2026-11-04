# Deteccion de dependencias cruzadas

Base e104034. Integracion principal aprueba extender el detector de ciclos
kernel a esperar_fin y enviar_esperando. Esperar_fin conserva -2 para ciclo;
enviar_esperando agrega -4 si un envio lleno con plazo formaria un ciclo.
No cambian formatos NXE, servicios CPL3, estados ni NEXTRY ASM.

## Comportamiento

Se recorre una cadena acotada a ocho tareas, mirando dependencias realmente
bloqueadas: ESPERANDO_FIN depende del resultado del destino y ESPERANDO_ENVIO
depende de que el destinatario libere su buzon. Una nueva espera que regrese
al origen se rechaza. La ampliacion ENTREGA-MUTEX.md incluye el propietario
de mutex en estas cadenas. El algoritmo comparte la validacion de IDs completos
para no confundir referencias antiguas con otra generacion del slot.

No se siguen dependencias retenidas por tareas ya listas tras un evento/plazo:
pueden reanudar sin esperar al destino. Tampoco se atribuye un destino concreto
a recibir mensajes, dormir, esperar semaforos sin propietario o tareas listas. No detecta protocolos arbitrarios,
fallos de callbacks ring0 ni bloqueos externos a estas dos operaciones.

Enviar sigue intentando encolar antes de revisar el ciclo. Con buzon disponible
no existe espera ni error nuevo. Con buzon lleno y cero ticks conserva consulta
sin bloqueo (1); con plazo devuelve -4 si generaria ciclo. Por eso una tarea
puede enviarse a si misma con espacio, pero no bloquearse sobre su buzon lleno.
Esperar_fin conserva su codigo -2 y sus resultados/timeout habituales.

No se crean objetos, registros dinamicos ni nuevas reservas de heap. Los
mensajes y los resultados se conservan al rechazar. Los plazos siguen
protegiendo las esperas que el detector no puede inferir.

## Demostracion

`tareas bloqueos` ejecuta tres casos: anillo de dos emisores, anillo de ocho
y pareja mixta de envio/finalizacion. Cada buzon empieza con cuatro valores
conocidos. Una dependencia que cerraria el anillo se rechaza y la finalizacion
despierta al resto. Se comprueban los codigos y los cuatro valores originales.
La limpieza cancela todas las dependencias antes de liberar slots.

La autoprueba adicional llena el buzon propio: consulta cero devuelve 1,
intentar esperar devuelve -4 y los mensajes previos permanecen intactos.
Las cadenas validas de ocho resultados y el flujo de dos productores siguen
siendo parte de las autopruebas para detectar falsos positivos.

La organizacion sigue siendo puente C cooperativo, una CPU y ring0, con el
cambio de contexto NEXTRY ASM existente. No se expone esta API a aplicaciones
CPL3 ni se agregan palabras al compilador NEXTRY en esta entrega.

Verificacion ejecutada: QEMU x86 32/64/128/1024 MiB, regresion x64 64 MiB,
consola general y del planificador, dieciseis escenarios NXE2. Dos repeticiones
de tareas bloqueos mantienen heap y guardas. Aplicaciones protegidas siguen
funcionando despues de las pruebas. El build exige el marcador CICLOS CRUZADOS OK.
