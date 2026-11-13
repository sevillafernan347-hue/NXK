# Cuotas de paginas privadas

Base4cb78de, contrato de integracion aprobado para v0.0.24.
Cada espacio comienza con cuota4 de paginas de datos (4096bytes cada una).
Solo puede reducirse, entre0 y4; un limite menor que el numero de paginas
ya reservadas se rechaza sin liberar ni cambiar datos. Liberar paginas no
recupera una cuota retirada. Una nueva generacion vuelve a4.

La API nxk_espacio_limitar_datos permite CR3 kernel o el mismo espacio, fuera
de tarea/IRQ:0 exito,-1 contrato/ID,-2 uso superior,-3 contexto,-5 ampliacion.
Consultar la cuota no cambia procesos ni plazos. La cuota afecta solo datos;
no cuenta codigo, pila ni tablas. Los espacios terminales conservan su cuota
hasta destruirse; el operador tambien puede reducirla si el uso lo permite.

Servicios opcionales21(EBX=limite) y22(consultar) solo para procesos gestionados.
21 devuelve status;22 limite0..4. Rutas antiguas devuelven-2. ABI1 sin cambios.
NEXTRY: limitar_memoria expresion_entera y cuota_memoria(). La orden falla
explicitamente si se rechaza la reduccion; reservar_pagina sigue retornando0
cuando no queda cuota o memoria. Consola: proceso memoria ID LIMITE.

Cuatro cuotas de tres bits se guardan en uint16, sin reservar heap ni aumentar
los PCB. Validacion estatica acota el numero de slots y el maximo codificable.
Puentes C de memoria/servicios; ejemplo NEXTRY. Una CPU y cuatro espacios,
sin cuotas de CPU, swap, crecimiento de pila ni reserva anticipada de RAM.

## Evidencia

85 pruebas del compilador superadas, incluidos tipos, aridad y ejemplo real.
Matriz QEMU x86 32/64/128/1024MiB y regresion x64 64MiB superadas. Imagen x86
normal77836B/test102412B; x64 normal24584B/test28680B. Se conserva reserva128KiB.

pruebas_cuotas_procesos.py verifica cuota2 agotada, rechazo bajo uso con datos
intactos, reduccion tras liberar, cuota0, ampliacion prohibida y permisos0.
Cuatro procesos con cuotas0/1/2/3 verifican aislamiento y limpieza al cancelar;
una generacion nueva vuelve4 y el ID viejo no modifica su limite. La consola
rechaza negativos/overflow y limites fuera del rango. Una recepcion suspendida
con buffer de dos paginas conserva sus datos/retenciones al rechazar cuota1.
El ejemplo real conserva datos tras ceder y termina42; NEXTRY con limite bajo
uso termina con error1 sin retirar la pagina. Las rutas antiguas rechazan las
extensiones. Heap/paginas estables y discos originales intactos con snapshot.

Regresiones pruebas_memoria_procesos.py y pruebas_archivos_procesos.py superadas.
