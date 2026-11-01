# Diagnostico de excepciones x86

Base: cc54d42. Entrada propia NEXTRY ASM y puente C.
La plantilla de 29 bytes normaliza el codigo de error, guarda registros y pasa
el contexto al manejador fatal con pila alineada. Cada vector CPU tiene su entrada.
Se informa vector, error, EIP y EAX/EBX/ECX/EDX por consola y serie.

Validacion: lenguaje/pruebas_excepciones.py provoca INT3 (3/error 0), UD2
(6/error 0) y selector invalido (13/error FFF8), comprueba la salida de QEMU
y que EIP pertenece al kernel; restaura la imagen normal al terminar.

Limites: no recupera la ejecucion, no dispone de pila de emergencia ni diagnostico
x64. Las entradas 32..255 usan un marcador comun; aun no hay servicio de IRQ.
Siguiente etapa: temporizador e interrupciones antes de introducir tareas.
