# NXK v0.0.16: tres servicios nuevos para procesos

Base v0.0.15, commit 074dfb2. Se agregan mensajes FIFO, recepcion bloqueante
con plazo y cancelacion sobre los cuatro procesos CPL3 existentes. Los PCB,
las copias y los servicios siguen usando puentes C; la entrada y reanudacion
conservan NEXTRY ASM. No se importa codigo de Linux ni de Windows.

## 1. Mensajes entre procesos

Cada proceso tiene cuatro posiciones de ocho bytes: origen y valor uint32.
Se entregan en orden FIFO. Un envio a una cola llena falla sin descartar ni
reemplazar mensajes; un destino terminado, fallido o cancelado rechaza envios.
El kernel registra el ID completo del emisor activo; la aplicacion no elige
su remitente. Origen 0 identifica la consola/kernel. El ID lleva generacion:
los mensajes para un ID liberado nunca llegan al siguiente ocupante del slot.

```text
funcion inicio
    entero propio = proceso_actual()
    entero valor = 0
    entero origen = 0
    entero resultado = 9
    enviar_mensaje propio, 42
    recibir_mensaje valor, origen, resultado, 0
    afirmar resultado == 0 y valor == 42 y origen == propio
fin
```

Ejemplo completo: `lenguaje/ejemplos/proceso_mensajes.nxf`. Las tres salidas
de recibir deben ser variables `entero` distintas y modificables. El destino,
el valor y el plazo son expresiones enteras. Logicos, textos, constantes como
salida y variables fuera de ambito se rechazan al compilar.

## 2. Recepcion con espera y plazo

`recibir_mensaje valor, origen, resultado, ticks` publica resultado 0 al recibir
o 1 si no hay mensaje. Con ticks 0 consulta sin suspender. Si la cola esta vacia
y ticks es positivo, conserva el contexto y pasa a ESPERANDO_MENSAJE (6).
No consume turnos hasta recibir un mensaje o vencer el plazo. Un resultado 1
conserva valor y origen; no inventa datos ni escribe parcialmente el registro.

El plazo se calcula durante INT81 y admite 0..0x7fffffff ticks PIT (~100 Hz).
Comparacion modular de 32 bits, con el mismo horizonte que `dormir`.
Al despertar se conserva el buffer virtual hasta el siguiente despacho. Con
CR3 kernel se copia el registro completo a su pila antes de retirar el mensaje.
Si hay mensaje al despachar, gana sobre el plazo, aunque ya haya vencido.
Una copia fallida no consume la cola y marca fallo del motor.

Ejemplo receptor: `lenguaje/ejemplos/proceso_receptor.nxf`. Compilar ambos:

```powershell
.\compilar-usuario.ps1 -Fuente lenguaje/ejemplos/proceso_mensajes.nxf -Salida build/x86/mensajes.nxe
.\compilar-usuario.ps1 -Fuente lenguaje/ejemplos/proceso_receptor.nxf -Salida build/x86/receptor.nxe
```

Incluirlos en un volumen NXFS como describe `lenguaje/FORMATO-NXE-USUARIO.md`.
Desde la consola NXK, despues de montar ese volumen:

```text
proceso cargar receptor.nxe
proceso ejecutar
proceso listar
proceso enviar ID 42
proceso ejecutar
proceso liberar ID
```

Sustituir ID por el numero mostrado al cargar. Si todos esperan, ejecutar
retorna 2 inmediatamente, igual que con los dormidos. Los envios despiertan
el receptor, pero no lo ejecutan dentro del envio. La consola sigue dirigiendo
las rondas: no hay despacho de aplicaciones mientras espera una orden.

## 3. Cancelacion y diagnostico

`proceso cancelar ID` acepta LISTO, DORMIDO o ESPERANDO_MENSAJE. Limpia la cola,
el buffer pendiente y el plazo; pasa a CANCELADO (7) y no vuelve a ejecutar.
Retiene el ID, la memoria y los contadores hasta `proceso liberar ID`.
No cancela un proceso ya terminal ni reutiliza su slot automaticamente.

`proceso listar` conserva los campos anteriores y agrega mensajes (ocupacion),
enviados, recibidos, esperas y plazos. Los cuatro ultimos saturan en uint32.
Los envios de la consola no incrementan enviados de otro proceso. Cancelar
no genera un resultado de CPU: el estado 7 es el diagnostico de cancelacion.

## Extension opcional de INT81

La cabecera NXE2 y ABI1 no cambian. Servicios anteriores conservan su contrato.
Los servicios nuevos solo existen en procesos gestionados; rutas sincronicas,
NXE1 y puentes kernel NEXTRY sin ese contexto fallan explicitamente.

| EAX | Entrada | Salida |
| --- | --- | --- |
| 11 | EBX destino completo, ECX valor | 0 enviado; -1 ID, -2 cola llena, -3 terminal |
| 12 | EBX buffer privado de 8 bytes, EDX plazo relativo | 0 recibido, 1 vacio/plazo, -1 contrato |
| 13 | Sin argumentos | ID completo del proceso actual |

El servicio 12 valida todo el rango de escritura en la pagina de pila y el
plazo antes de consumir un mensaje. Rechaza codigo RO, direcciones kernel,
overflow y registros que crucen el final de la pagina. En espera conserva
GPR, segmentos, EIP/ESP y banderas con la normalizacion existente. El retorno
interno usa motivo 4 y publica el plazo por el argumento ya existente del motor.

## Validacion y trabajo restante

Pruebas del compilador: `lenguaje/pruebas.py` y
`lenguaje/pruebas_ipc_compilador.py`. Integracion:
`lenguaje/pruebas_mensajes.py`, matriz `test.ps1 -Architecture x86 -MemoryMatrix`
y regresiones de procesos, dormir, preempcion y NXE2. Los resultados concretos
se registran en CAMBIOS.md y ESTADO.md despues de ejecutar las pruebas.

Limites: una CPU, cuatro procesos, cuatro mensajes de un uint32 por cola,
envio sin bloqueo, sin ACL ni canales con permisos. Los archivos siguen
compartidos; faltan espera de dispositivos, recursos por proceso y contexto
FPU. El kernel y sus llamadas al sistema siguen cooperativos. No se certifica
hardware ASUS ni se asigna porcentaje comparado con Windows.
