# Proteccion de escritura de datos privados

Base ac7a1ac (v0.0.25). Contrato aprobado por integracion para v0.0.26.

Solo las cuatro paginas de datos reservadas admiten cambiar escritura:0 solo
lectura,1 lectura/escritura. Codigo y pila mantienen sus contratos anteriores.
Cada reserva nueva comienza RW y en cero. La proteccion persiste al ceder,
dormir y reactivar el espacio; no modifica su contenido ni su cuota.

API nxk_espacio_proteger_paginas(id,inicio,cantidad,escritura): rango alineado
de1..4 paginas existentes. Valida todo antes de cambiar ninguna.0 exito,
-1 ID/rango/hueco/bandera,-3 contexto,-5 al retirar escritura de una pagina
retenida como salida IPC. El rechazo conserva todos los permisos. Solo CR3
kernel o propio, fuera de tareas/IRQ; no cambia CR3 ni asigna memoria.
API nxk_espacio_escritura(id,inicio,&salida): direccion exacta de pagina de datos
reservada; devuelve0 y publica0/1, mismos errores-1/-3 y salida intacta al fallar.

El validador y las copias kernel rechazan rangos de escritura que incluyan RO
antes de copiar o consumir efectos. La CPU impide escrituras CPL3. Lecturas,
salida de texto y guardado de archivos desde RO siguen permitidos. No existe
proteccion NX en este paging x86 sin PAE: RO no significa no ejecutable.

INT81/25: EBX=inicio ECX=cantidad EDX=escritura, devuelve0,-1 o-5.
INT81/26: EBX=inicio, devuelve0 RO,1 RW o-1. Solo procesos gestionados; ruta
legacy devuelve-2 no soportado. NEXTRY proteger_paginas(base,n,escritura) y
escritura_pagina(base) son expresiones entero con los mismos retornos;
un servicio no soportado causa error explicito del runtime.

Consola proceso proteger ID INICIO CANTIDAD ESCRITURA para un proceso gestionado.
No se amplia el espacio virtual, el numero de procesos ni el ABI NXE1.
Puente C pendiente de migracion a NEXTRY. Una CPU, sin memoria compartida.

## Reserva de variables revisada

Integracion aprueba ampliar exclusivamente imagen+BSS de128 a192KiB, con fin
menor de0x40000. La imagen de pruebas con la cobertura completa supera128KiB.
No hay consumidores en0x30000..0x40000; la entrada NXA limpia BSS mediante
simbolos, sin limite fijo. Archivo BIOS conserva120KiB/fin0x2e000; pila90000,
heap100000 y demas regiones no se mueven. Ambas imagenes conservan -Os.
El bitmap RO usa2bytes, no heap. Consulta MAPA-MEMORIA.md.

## Pruebas ejecutadas

- 95 pruebas del compilador NEXTRY:71 extensiones y24 generales, superadas.
  Ejemplo proteccion_memoria.nxf compilado a NXE2 y ejecutado hasta codigo42.
- Matriz QEMU x86 32/64/128/1024MiB y regresion x64 64MiB superadas, con todas
  las comprobaciones internas anteriores y las nuevas de proteccion.
- Prueba interna: copia cruzada RW/RO rechazada sin cambiar la parte RW,
  consulta conserva salida al fallar, contexto ajeno rechazado, reactivacion
  conserva PTE RO, liberar/reservar devuelve paginas en cero y RW.
- pruebas_proteccion_procesos.py: fallos CPU14/error7 contenidos tanto justo
  despues de proteger como tras ceder; reabrir RW funciona sin otro despacho.
  Rangos/huecos/overflow/banderas invalidos no aplican proteccion parcial.
  Dos procesos mantienen politicas independientes en la misma direccion virtual.
- Salida y guardado de archivos desde RO permitidos; lectura de archivos,
  recepcion y FIN con destino cruzado RW/RO rechazados sin copiar parcialmente
  ni consumir mensajes. Tras reabrir se recuperan mensaje y resultado esperados.
- Retenciones de recepcion/FIN rechazan proteger un rango cuya primera pagina
  no esta retenida y otra si; todas conservan RW. Incluye LISTO tras plazo,
  entrega, cancelacion, ID obsoleto y entradas invalidas de consola.
- Rutas legacy rechazan25/26; el runtime NEXTRY no finge soporte.
- Regresiones pruebas_memoria_procesos.py, pruebas_buffers_procesos.py y
  pruebas_lotes_procesos.py superadas. Heap/paginas recuperados y discos
  originales intactos; cada prueba externa usa su propia imagen snapshot.
- pruebas_limites_kernel.py: cinco enlaces reales comprueban BSS por encima
  del limite antiguo, rechazo de BSS excesiva, archivo en el limite y rechazo
  de codigo/datos que excedan120KiB.

Tamano de archivo: x86 normal77836B/pruebas106508B; x64 normal24584B/
pruebas28680B. No equivale al uso total de RAM. Se conserva -Os, las aserciones
del linker y la cobertura anterior. Revision estatica adicional sin hallazgos
bloqueantes; no garantiza ausencia de errores ni certifica la ASUS fisica.
