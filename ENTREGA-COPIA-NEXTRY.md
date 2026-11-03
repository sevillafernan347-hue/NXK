# Copias desde aplicaciones NEXTRY

Base: 8f74ea5. La cuenta principal integra la ampliacion.
Contrato aprobado por integracion: servicio INT 0x81 numero 8, EBX nombre
origen, ECX nombre destino, ambas cadenas NUL de hasta 23 caracteres en
memoria usuario. Retorna cero o FFFFFFFF. No cambia el formato NXE ni los
servicios existentes. Solo crea archivos RAM nuevos, hasta 256 bytes.

Se ampliaron compilador, runtime CPL3 y puente kernel con copiar_archivo.
El runtime NXE1 antiguo rechaza explicitamente esta orden; su tabla API
no se modifica. Los nombres se copian a buffers kernel antes de operar.

Cinco casos de puente adicionales: copia valida, colision, destino kernel,
destino sin terminador y origen kernel. Se comprueba contenido original y
copia tras cada rechazo, sin crecimiento del heap. La app integrada copia y
borra el original antes de mostrar su copia.

Pruebas ejecutadas: 22 de compilador; matriz x86 32/64/128/1024 MiB; regresion
x64 64 MiB. Diez escenarios de copia NEXTRY real: normal, vacio, exactamente
256, 257 rechazado, contenido corrupto, origen ausente, destino disco, destino
RAM, tabla RAM llena y volumen sin montar. Consola recuperable e imagenes base
intactas byte a byte en todos los escenarios. Los fallos no publican destinos.

El soporte sigue usando puentes C y el runtime generado; no es un sistema de
archivos persistente ni una implementacion concurrente.

Ejemplo versionado: lenguaje/ejemplos/copiar_volumen.nxf. Se integra con
build.ps1 -FuenteUsuario lenguaje/ejemplos/copiar_volumen.nxf, o se compila
independientemente con compilar-usuario.ps1 -Fuente y -Salida para NXE2.
Montar NXFS antes de ejecutar; la consola puede editar la copia resultante.

Cierre de regresiones: once fuentes negativas contienen el error y recuperan
consola; dieciseis escenarios NXE2 superados. El ejemplo NXE2 independiente
copia, edita, muestra y borra un archivo del volumen; tres recargas exitosas
con el mismo heap. Prueba de consola general superada con la app por defecto.
