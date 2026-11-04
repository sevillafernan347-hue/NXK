# Resumen del planificador y versiones publicadas

Base 358156c. Integracion principal amplia solo la interfaz kernel con una
consulta global del planificador, sin modificar NXE ni servicios CPL3.
Se inicia la serie de etiquetas v0.0.1 (entrega previa) y v0.0.2 (esta entrega).
El historial de commits anterior se conserva completo; no se reescribe.

La instantanea cuenta libres y estados registrados; pendientes incluye tareas
listas, ejecutando, dormidas y las tres esperas. Finalizadas suma terminadas y
canceladas que aun no se liberaron. Mensajes incluye los retenidos en tareas
terminales, hasta liberar su buzon. Reservas de pila cuentan slots con reserva
permanente de 12 KiB: limpiar tareas no devuelve esas reservas al heap monotono.
La consulta no es un porcentaje de CPU ni un medidor de RAM fisica libre.

Se copia el resumen al llamador solo al completar: salida nula devuelve -1 y
estado/cantidad de buzon invalidos -2, sin modificar la salida. Sin asignaciones
nuevas ni cambios de estado. Una CPU kernel fuera de IRQ; x64 retorna -64.
La implementacion es puente C y conserva el cambio de contexto NEXTRY ASM.

Pruebas: resumen vacio al arrancar, ocho tareas pendientes, siete terminadas y
una cancelada, liberacion con ocho reservas conservadas, y consultas repetidas
sin mutacion. Dentro de una tarea se comprueban estado ejecutando y cuatro
mensajes en su buzon. Una corrupcion controlada de estado confirma salida
intacta y evita usar un indice fuera de rango. La consola verifica la version
leyendo su valor esperado desde lib/version.h.

Verificacion ejecutada: QEMU x86 32/64/128/1024 MiB, regresion x64 64 MiB,
consola general/del planificador y dieciseis escenarios NXE2. La consola
confirma que version coincide con lib/version.h y que consultar no altera
resumen, estadisticas ni uso de heap/guardas.
