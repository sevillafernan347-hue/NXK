# Historial acotado de ejecuciones de usuario

Ocho registros en anillo, copiados mediante lib/usuario.h. Cada registro informa
secuencia uint32 (puede dar vuelta), salida, indicador de fallo, vector, error,
EIP y CR2. No es identificador de proceso ni registro persistente. No asigna
memoria adicional por ejecucion; al llenarse reemplaza el mas antiguo.

Comandos: usuario estado muestra el ultimo; usuario historial muestra hasta
ocho, del mas reciente al antiguo; usuario nulo provoca un acceso nulo conocido
y lo contiene. usuario ejecuta el caso correcto. Un resultado correcto limpia
campos de fallo; vector cero no se usa como indicador de exito (division cero
es una excepcion valida). Fuera de ejecuciones iniciadas no se registra nada.

Pruebas: registro de fallos, limpieza tras exito, desbordamiento del anillo,
rechazo de indices fuera de rango y destino nulo; no crecimiento del heap.
Matriz x86 y regresion x64; consola serie comprueba estado vacio, fallo nulo,
estado correcto posterior, historial y help. No representa procesos generales,
no hay preempcion ni concurrencia y las apps NXE habituales siguen en ring0.
