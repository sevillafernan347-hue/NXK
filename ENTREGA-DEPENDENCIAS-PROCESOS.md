# NXK v0.0.17: dependencias y resultados de procesos

Base: v0.0.16, commit e755e00. Contrato propuesto y aprobado por la sesion
principal antes de integrar las tareas paralelas. Las aplicaciones anteriores
y la cabecera NXE2 ABI1 conservan sus contratos. Nuevos puentes C en el gestor
y servicios; entrada/reanudacion NEXTRY ASM existente.

## Contrato de integracion

- Envio con espera: servicio INT81/14, EBX destino completo, ECX valor,
  EDX ticks. Retorna 0 enviado, 1 cola llena con plazo0 o plazo vencido;
  -1 ID/contrato, -3 destino terminal, -4 ciclo de dependencias.
- Esperar fin: INT81/15, EBX destino completo, ECX buffer privado8bytes
  {estado,codigo}, EDX ticks. Retorna 0 resultado disponible, 1 consulta
  no terminal/plazo vencido, -1 ID/contrato o -4 ciclo.
- Intervalos 0..0x7fffffff. Consulta0 nunca suspende. Resultado/espacio
  disponible al despachar gana sobre el plazo, como la recepcion existente.
- Estados nuevos8 ESPERANDO_ENVIO y9 ESPERANDO_FIN; motivos internos5/6.
  El motor conserva GPR/segmentos/marco y publica plazo por el quinto argumento.
- Retorno interno2 de preparar significa suspender y no se expone a CPL3.
  El marco sale con EAX1 provisional; el gestor lo actualiza antes de reanudar.
- Esperar fin retiene el destino hasta entregar, vencer al despachar o cancelar.
  Liberar por proceso o por espacio devuelve -5 si conserva observadores.
  No se consume el resultado: varios observadores pueden recibirlo.
- Pendiente de envio conserva valor e ID con generacion; no retiene destino.
  Si desaparece, retorna -1 sin entregar a una nueva generacion del mismo slot.
- Se rechazan ciclos por aristas de envio lleno/fin pendientes y aun bloqueantes,
  al registrar y al volver a despachar una espera.
  Recepcion sin emisor fijo no expresa una dependencia en este detector.
- Finalizar con codigo usa INT80 existente, sin una nueva llamada al sistema.

Las instrucciones NEXTRY nuevas son `enviar_esperando`, `esperar_proceso` y
`finalizar`. Se conservan `enviar_mensaje`, `recibir_mensaje`, `ceder` y `dormir`.

## Enviar sin perder mensajes cuando la cola esta llena

```text
entero resultado = 9
enviar_esperando receptor, 42, resultado, 100
si resultado == 0
    mostrar "Mensaje enviado"
sino
    afirmar resultado == 1
    mostrar "No hubo espacio antes del plazo"
fin
```

La cola sigue teniendo cuatro posiciones. Si tiene espacio, se envia en ese
turno; con plazo0 y cola llena se retorna1 sin suspender. En la espera positiva
el PCB conserva el valor y el ID completo del receptor, sin seguir ejecutando
la aplicacion. El planificador vuelve a comprobar el destino y el hueco antes
de entregar, incrementando `enviados` una sola vez al encolar.

Despertar no reserva una posicion de la cola. Si otro emisor ocupa el hueco,
el primero vuelve a esperar con el plazo original, sin incrementar sus turnos
CPU ni registrar otra espera. No hay garantia de orden entre varios emisores
esperando espacio; los mensajes ya encolados mantienen su FIFO. Un receptor
cancelado/terminado produce error; un ID liberado produce error aunque el
mismo slot se haya reutilizado para otra aplicacion.

## Observar un resultado y terminar con codigo

```text
entero estado = 99
entero codigo = 99
entero resultado = 9
esperar_proceso trabajador, estado, codigo, resultado, 100
si resultado == 0
    afirmar estado == 3
    numero codigo
sino
    afirmar resultado == 1
    afirmar estado == 99 y codigo == 99
fin
```

El trabajador puede terminar con `finalizar 42`. Esta instruccion no vuelve;
un retorno normal de `inicio` conserva el codigo0 anterior. En modo usuario
se usa INT80, que ya soportaba un codigo uint32. El puente NEXTRY kernel
delegara a la tarea actual; fuera de una tarea falla. NXE1 carece de salida
no local con codigo y la rechaza explicitamente.

| Estado recibido | Significado de codigo |
| --- | --- |
| 3 TERMINADO | Codigo uint32 de salida de la aplicacion |
| 4 FALLIDO | Resultado del fallo CPU; UINT32_MAX si fallo el motor |
| 7 CANCELADO | UINT32_MAX; no se presenta como una salida normal |

Una consulta con plazo0 retorna1 mientras el destino siga vivo y no retiene
una referencia. Una espera positiva conserva el resultado hasta copiar los
ocho bytes al observador. Varios observadores pueden recibir el mismo resultado:
no es una extraccion ni libera automaticamente al trabajador.

La liberacion del destino se rechaza con -5 mientras queden referencias, incluso
si el plazo del observador vencio pero aun no recibio turno. Despachar, cancelar
o liberar al observador retira su referencia. El control existe tanto en la
API de procesos como en `nxk_espacio_destruir`, usada por `usuario liberar`.
Cancelar un trabajador permite observar estado7; no elimina el resultado antes
de que lo reciban sus observadores.

## Ciclos y plazos

El detector recorre como maximo los cuatro PCB con IDs completos, sin recursion.
Envio a cola llena y espera de finalizacion forman las aristas. Una dependencia
cuyo plazo vencio, cuyo destino es terminal o cuya cola ya tiene espacio no
esta bloqueando y no se usa para rechazar otra peticion.

Se comprueba al preparar la espera y de nuevo al recorrer/despachar pendientes.
Esto cubre un ciclo que aparece despues: A espera espacio en B; B abre un hueco
y espera el fin de A; un tercero llena otra vez B antes de que A ejecute.
El siguiente despacho afectado recibe -4, incrementa `ciclos` y retira su
dependencia. No se espera hasta vencer un intervalo largo para romper ese ciclo.

Las esperas de recepcion sin emisor concreto no expresan una arista hacia un ID.
El detector no puede inferir todas las dependencias del programa ni las que
pasen por dispositivos, archivos o protocolos propios. Los plazos siguen
limitados a media vuelta del reloj32, con las limitaciones de dormir.

## Probarlo desde la consola

`proceso dependencias` crea un trabajador y un observador privados. Comprueba
que no se pueda liberar al trabajador por ninguna de las dos API antes de
entregar {TERMINADO,42}, y despues libera ambos. Rechaza ejecutarse si hay
procesos registrados, para no interferir con aplicaciones ajenas.

Los ejemplos comentados son `lenguaje/ejemplos/proceso_productor.nxf`,
`proceso_consumidor.nxf` y `proceso_observador.nxf`. Compilarlos con
`compilar-usuario.ps1 -Fuente RUTA -Salida build/x86/NOMBRE.nxe` e incluirlos
en NXFS como las otras aplicaciones. Tras cargar los tres:

```text
proceso enviar ID_PRODUCTOR ID_CONSUMIDOR
proceso enviar ID_OBSERVADOR ID_PRODUCTOR
proceso ejecutar
proceso listar
```

Sustituir los IDs por los mostrados al cargar. El productor envia doce valores;
el consumidor verifica el orden y suma78. El observador espera la salida42 del
productor. Volver a ejecutar si se agota el presupuesto; liberar primero los
observadores una vez finalizados y despues los trabajadores.

`proceso listar` agrega `esperando`, `esperas_envio`, `esperas_fin` y `ciclos`.
`esperando` es el ID al que apunta el pendiente, no un contador; los otros
campos saturan. El planificador devuelve2 si solo quedan esperas y conserva
el control en la consola. No despacha aplicaciones en segundo plano mientras
la consola espera una orden.

## Pruebas y limites

`lenguaje/pruebas_dependencias.py` comprueba las llamadas directamente con
NXE pequenos y tambien compila NEXTRY. Incluye FIFO, cola llena, plazos,
registro de resultado intacto en errores, GPR/CF/DF, IDs antiguos, referencias,
dos observadores, cancelacion y fallos, ciclos de dos/tres procesos y ciclos
mixtos/dinamicos. `lenguaje/pruebas_dependencias_compilador.py` comprueba tipos,
constantes, alias, ambitos y forma de las nuevas instrucciones.

El arranque de pruebas ejecuta la demostracion tres veces, comprobando paginas,
heap, guardas y CR3. Los resultados ejecutados, junto a la matriz de RAM y las
regresiones, se registran en CAMBIOS.md y ESTADO.md al cerrar la entrega.
La prueba externa tambien ejecuta los tres ejemplos del repositorio juntos:
doce mensajes, suma78 y salidas42/42/0; comprueba el rebloqueo si se pierde un
hueco sin formar un ciclo y repite el comando de demostracion sin crecer memoria.

Siguen siendo cuatro procesos, una CPU, mensajes de un valor uint32, archivos
compartidos sin permisos y rondas dirigidas desde la consola. No hay espera de
dispositivos, SMP ni contexto FPU. Las pruebas QEMU no certifican una ASUS fisica.
