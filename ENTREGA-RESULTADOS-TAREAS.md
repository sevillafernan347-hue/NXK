# Resultados y espera de finalizacion de tareas

Base 35802c4. Integracion principal aprueba ampliar solo la API kernel:
finalizar(codigo), esperar_fin(id,resultado,ticks), estado ESPERANDO_FIN
agregado al final, resultados y dependencia en la consulta. Sin cambios a NXE
ni INT 0x81. Puente C con cambio de contexto NEXTRY ASM existente.

## Interfaz y ciclo de vida

nxk_tarea_finalizar(codigo) termina la tarea actual sin volver a su callback.
Retornar normalmente del callback sigue siendo valido y publica codigo cero.
Cancelar publica estado CANCELADA y codigo UINT32_MAX; el estado permite
distinguir cancelacion de una tarea que elija ese mismo codigo al terminar.

nxk_tarea_esperar_fin(id,&resultado,ticks) devuelve cero si el destino ya es
terminal o cuando termina, uno por plazo/consulta no disponible, y negativos
por contrato o dependencia circular. El resultado tiene estado y codigo; no
se consume, asi que varios observadores pueden leer el mismo resultado.
Cero ticks consulta sin bloquear. Salida intacta ante error o vencimiento.
Solo puede esperar una tarea kernel activa con interrupciones habilitadas.

ESPERANDO_FIN es un estado nuevo al final de la enumeracion. El organizador no
le da turnos hasta terminar/cancelar el destino o vencer el plazo. Al reanudar
prima un resultado terminal disponible sobre el plazo: no es tiempo real.
La consulta publica resultado y el ID del destino esperado; tareas listar
muestra los codigos terminales y las dependencias que sigan referenciadas.

Una referencia se conserva incluso si el observador ya fue despertado, hasta
que reanude y recoja el resultado o se cancele. Liberar el destino devuelve -3
si todavia hay referencias. Cancelar un observador libera su dependencia y
puede despertar a sus propios observadores. No se reutilizan slots bajo una
espera pendiente. Se rechazan autoespera y cadenas que crearian un ciclo.
Esto solo detecta ciclos de esperar_fin, no protocolos arbitrarios por mensajes.

## Demostracion

`tareas resultados` crea un trabajador y dos observadores. Ambos esperan antes
de que el trabajador finalice con 42. El primer observador comprueba que no se
puede liberar el resultado antes de reanudar el segundo. Ambos lo consultan de
nuevo sin consumirlo. Se cancelan dependencias antes de liberar en la limpieza
de errores para no dejar argumentos de pila referenciados.

## Verificacion

Las autopruebas comprueban: retorno normal/explicito, dos observadores, lectura
repetida, liberacion protegida, timeout sin alterar salida, consulta cero,
parametros invalidos, autoespera y un ciclo entre dos tareas. Se cancela un
destino mientras otro espera, se verifica su resultado CANCELADA, y se cancela
un observador antes de liberar su destino. La continuacion cancelada no corre.

Una cadena de las ocho tareas espera al siguiente eslabon antes de resolver
la ultima con 100; se propagan 101..107, se revisan estados/dependencias y se
liberan todos los slots. Se repiten las demostraciones sin crecer heap/guardas.

El mecanismo sigue siendo cooperativo, de una CPU y ring0. No son hilos CPL3,
no hay API NEXTRY nueva ni finalizadores de recursos al cancelar. La
organizacion permanece en C y conserva el cambio de contexto NEXTRY ASM.

Verificacion ejecutada: matriz QEMU x86 de 32, 64, 128 y 1024 MiB; regresion
x64 64 MiB; consola general y consola de planificador (incluye dos demos de
resultados y memoria estable); dieciseis escenarios NXE2 desde disco.
