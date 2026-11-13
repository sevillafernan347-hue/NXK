# Lotes de paginas privadas

Base4d504cd. Contrato aprobado por integracion para v0.0.25.
Reservar1..4 paginas devuelve un rango virtual contiguo, con respaldos fisicos
independientes y en cero. La cuota se verifica antes de adquirir paginas.
Si no hay rango virtual libre o memoria fisica suficiente, no publica ninguna
pagina y devuelve todas las temporales. La salida kernel permanece intacta.

Liberar un rango valida primero todos sus miembros y las retenciones IPC.
Un hueco, limite invalido o pagina retenida rechaza todo sin liberar parcialmente.
Las API de una pagina delegan al mismo motor. No hay identidad de bloque: un
rango puede incluir paginas reservadas mediante llamadas independientes.
Las API kernel exigen CR3 kernel o propio y conservan los contratos anteriores.
Se presupone asignador fisico y llamadores ring0 confiables; no se promete
rollback ante corrupcion de sus metadatos durante una liberacion.

INT81/23 EBX=cantidad devuelve inicio o0 al rechazar/faltar espacio.
INT81/24 EBX=inicio ECX=cantidad devuelve0,-1contrato o-5retencion. Rutas
antiguas devuelven-2 no soportado. Solo procesos gestionados; ABI1 sin cambios.
NEXTRY reservar_paginas(cantidad) y liberar_paginas(inicio,cantidad) permiten
examinar los retornos. La reserva valida y la cuota no garantizan una corrida
virtual: pueden existir huecos separados. El maximo sigue siendo cuatro paginas.
Puentes C, una CPU, sin memoria compartida, swap ni asignacion por bytes.

## Pruebas ejecutadas

90 pruebas compilador superadas; matriz QEMU x86 32/64/128/1024MiB y regresion
x64 64MiB superadas. Imagen x86 normal77836B/test102412B; x64 normal24584B/
test28680B. Funciones de prueba y helpers de cuotas prefieren tamano para
conservar128KiB sin retirar aserciones ni ampliar memoria reservada.

En32MiB se agota el asignador real dejando solo una pagina disponible y se
piden dos: error-4, salida intacta, ninguna PTE publicada y temporales devueltas.
La cadena temporal de paginas se libera y el contador original se recupera.
La prueba se omite en RAM mayor para acotar el tiempo de arranque.
Recepcion y FIN comprueban un rango con segunda pagina retenida: devuelve-5
y ambas siguen accesibles. Se conserva la cobertura anterior de retenciones.

pruebas_lotes_procesos.py superada: cantidades/rangos invalidos, falta de cuota,
huecos, fragmentacion, paginas en cero, mezcla de reservas individuales y lotes,
datos conservados tras ceder, dos procesos, cancelacion y destructor recuperan
recursos. Ejemplo NEXTRY real y ruta legacy sin soporte verificados. Memoria y
discos originales permanecen estables usando snapshot.

Regresiones pruebas_memoria_procesos.py y pruebas_cuotas_procesos.py superadas,
incluida recepcion con buffer de dos paginas retenidas y limites por operador.
