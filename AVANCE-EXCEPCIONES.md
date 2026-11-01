# Primer manejo de excepciones x86

Actualizacion: la entrada de excepciones ahora se genera desde `kernel/excepciones32.nxa` con NEXTRY ASM. El archivo anterior `excepciones32.S` queda como referencia y ya no participa en la construccion. El nuevo stub genera 19 bytes; el inicializador instala la direccion del manejador en su inmediato de 32 bits antes de cargar la IDT. La tabla y el diagnostico siguen en C. Se verificaron nuevamente el arranque en cuatro cantidades de RAM y la captura de INT3.

Se incorpora una IDT de 256 entradas para NXK de 32 bits. Todas conducen a un manejador fatal comun que informa por la consola y detiene la CPU. Se usa el selector de codigo activo del cargador.

Se verifican el arranque normal y una excepcion INT3 provocada en una imagen especial de pruebas. Todavia no se identifican el vector ni los registros en el mensaje, no se reanuda la aplicacion y no se recuperan procesos. No hay pila de emergencia para doble fallo. Las interrupciones externas permanecen deshabilitadas. La variante x64 aun necesita su IDT propia.

Esta etapa se implementa provisionalmente en C y ensamblador anterior: no satisface aun el requisito de implementar todo en NEXTRY. Debe migrarse cuando NEXTRY soporte estructuras binarias, operandos de memoria y las instrucciones necesarias. No se presenta este avance como un kernel completo ni equivalente a Windows.

Siguientes capacidades: informar vector y registros, ampliar NEXTRY para expresar estos componentes, mapa fisico de memoria, asignador de paginas, temporizador y planificador.
