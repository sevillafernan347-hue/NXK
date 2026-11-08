# NXK v0.0.19: permisos por proceso

Base v0.0.18. La sesion principal aprueba los nuevos permisos del PCB y los
servicios opcionales INT81/16..17. NXE2 conserva cabecera y ABI1. El gestor y
las comprobaciones son puentes C; entrada y reanudacion siguen en NEXTRY ASM.

## Contrato y alcance

| Bit | Operacion permitida |
| --- | --- |
| 1 | Salida de texto por INT81/1 |
| 2 | Leer archivos mediante INT81/3 y5 |
| 4 | Crear, modificar, borrar y renombrar archivos mediante INT81/2,4,6,7 |
| 8 | Enviar mensajes a procesos, inmediatamente o con espera |

Copiar un archivo (servicio8) exige lectura y modificacion: mascara6.
Cada proceso nuevo empieza con mascara15 para conservar las aplicaciones
anteriores. Una restriccion solo puede quitar bits; ni el proceso ni la consola
pueden restaurarlos sobre ese ID. Mascara0 es valida y repetir la misma mascara
es valido. Bits desconocidos se rechazan. Crear un proceso nuevo asigna15 junto
a un nuevo ID de generacion; un ID antiguo no modifica a su sucesor.

La consola ofrece `proceso permisos ID MASCARA`, con valores decimales. Acepta
procesos vivos fuera del ejecutor y muestra la mascara en `proceso listar`.
La consola del kernel es un llamador de confianza: sus operaciones sobre archivos
y mensajes no pertenecen a los permisos de otro proceso.

La comprobacion de archivos/salida ocurre antes de leer buffers o cambiar datos.
Los envios se validan tambien en el gestor; al revisar un emisor que espera
espacio se detecta la revocacion del bit8 y vuelve a LISTO. Al despacharlo retorna
-5 antes de encolar. Esa dependencia ya
no forma parte de los ciclos de espera. No se descarta ni entrega el mensaje
pendiente despues de revocar su permiso.

INT81/16 recibe EBX=mascara y retorna0, -1 si el contrato es invalido o -5
al intentar restaurar permisos. INT81/17 retorna la mascara actual, incluido0.
Solo estan disponibles en la ruta de procesos gestionados. Una operacion
prohibida retorna -5; los runtimes NEXTRY terminan explicitamente con error
cuando esa instruccion exige exito. Dormir, recibir, esperar resultados y
finalizar siguen disponibles aun con mascara0.

NEXTRY incorpora `restringir_permisos expresion_entera` y `permisos_actuales()`.
NXE1 y el puente kernel fuera de procesos rechazan estas operaciones; no simulan
una proteccion inexistente. El ejemplo comentado
`lenguaje/ejemplos/proceso_permisos.nxf` comprueba15, renuncia a escritura/envio
con3, repite la restriccion y finalmente renuncia a todo con0 antes de salir42.

```text
funcion inicio
    afirmar permisos_actuales() == 15
    restringir_permisos 3
    afirmar permisos_actuales() == 3
    mostrar "Solo salida y lectura"
    finalizar 42
fin
```

Para reducir desde la consola, cargar el NXE2 como proceso y ejecutar
`proceso permisos ID 3` antes de darle un turno. Sustituir ID por el numero
mostrado al cargar. `proceso listar` permite consultar la mascara retenida,
incluso despues de terminar o cancelar. Un terminal rechaza nuevas restricciones.
La mascara se conserva durante cesiones, esperas y preempcion.

La reduccion no borra datos ya leidos ni retira mensajes previamente encolados.
Las operaciones futuras y los envios aun pendientes se vuelven a validar.

## Pruebas

Pruebas internas de defaults, subconjuntos, idempotencia, mascara0, estados
terminales y generacion nueva. `lenguaje/pruebas_permisos.py` usa actores NXE
que comprueban los retornos sin esconderlos tras el compilador. Verifica
salida denegada sin texto, archivos sin modificaciones, lectura denegada con
buffer intacto, copia que exige lectura y escritura, IPC inmediato y suspendido,
revocacion sin entrega ni ciclo falso y rutas antiguas sin las extensiones.
Tambien ejecuta el ejemplo NEXTRY real y compara memoria y discos originales.
Las pruebas del compilador comprueban tipos, sintaxis y consulta como entero.

Los resultados ejecutados y la matriz de arranque se registran en CAMBIOS.md
y ESTADO.md al publicar. Las pruebas virtuales no certifican hardware ASUS.

Son permisos generales por operacion, no permisos por archivo, usuarios o canales.
No separan directorios ni nombres; los archivos siguen compartidos. Tampoco
convierten las rutas antiguas ring0 o CPL3 sin PCB en procesos restringidos.
