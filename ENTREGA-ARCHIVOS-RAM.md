# Archivos en RAM propios

Ocho archivos, 256 bytes por archivo, nombres de 1 a 23 caracteres ASCII
minusculos, numeros, punto, guion y guion bajo. API en lib/archivos.h;
implementacion original en kernel/archivos.c (puente C pendiente de NEXTRY).
Comandos: escribir nombre texto, leer nombre, borrar nombre y archivos.
No hay persistencia: al reiniciar se pierde todo. Sin directorios, permisos,
disco ni concurrencia. La consola limita cada linea a 79 caracteres.

Pruebas de API: limite de contenido, lectura insuficiente, preservacion tras
rechazo, borrado, tabla llena, nombre invalido y reutilizacion de entrada vacia.
Matriz x86 32/64/128/1024 MiB superada. Regresion x64 64 MiB superada.
Los comandos interactivos se compilaron; no se automatizo su entrada en consola.
Compilacion cambiada de O2 a Os para respetar el limite del cargador: imagen
x86 de pruebas de 19600 bytes. No se cambiaron direcciones ni ABI.

Estudio Linux: dependencias de kernel/sched/core.c separan planificacion,
memoria, interrupciones y sincronizacion. NXK mantiene limites por subsistema
y validacion antes de modificar datos. No se copio codigo de Linux. Esta
entrega no acredita superioridad ni porcentaje comparado con Linux/Windows.
La licencia NXK definitiva sigue pendiente de elegir sus permisos.
