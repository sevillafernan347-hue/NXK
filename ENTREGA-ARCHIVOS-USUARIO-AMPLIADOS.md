# Agregar contenido y renombrar desde NEXTRY protegido

Ordenes: agregar_archivo "nota", " mas texto" y renombrar_archivo "nota", "nuevo".
Compilador exige dos textos. Runtime CPL3 usa INT 0x81 servicio 6 para agregar
(EBX nombre, ECX datos, EDX bytes hasta 256) y 7 para renombrar (EBX nombre,
ECX nuevo nombre). EAX cero en exito, FFFFFFFF error. Se copian nombres/datos
validados antes de llamar al gestor RAM; no se escribe al volumen NXFS.

Agregar exige archivo RAM existente y espacio hasta 256 bytes totales; falla
sin truncar ni modificar. Renombrar no reemplaza otro archivo y conserva datos;
el mismo nombre es una operacion sin cambios. Nombres hasta 23 caracteres.
Runtime de texto limita cada argumento de contenido a 255 bytes. Los errores
terminan la app con 1 y conservan consola. Tambien hay puentes para modulo
kernel; ABI NXE 1 anterior no incorpora estas operaciones y falla explicitamente.

Pruebas raw: agregar CD a AB, puntero kernel rechazado, 257 bytes rechazados,
capacidad total excedida, nombre de destino existente, cambio correcto y puntero
invalido del nombre nuevo. Despues de cada caso se comprueba ABCD y que el
archivo de destino previo siga vacio; fixtures no sobrescriben nombres previos.
Pruebas NEXTRY: archivo ausente, 255+2 bytes y renombrado duplicado fallan sin
perder la consola. Ejemplo usuario.nxf agrega, renombra, muestra y borra.
21 pruebas de compilador, ocho fuentes negativas, dieciseis NXE2 desde disco,
matriz x86 32/64/128/1024 MiB, consola y regresion x64 superadas.

El enlace de apps usa secciones por funcion/dato y --gc-sections. Medicion con
los mismos objetos del ejemplo: 726 bytes de codigo/constantes frente a 823 sin
GC; contenedor NXE2 agrega 64 bytes. No cambia ABI ni permisos; deja mas margen
dentro de 4032 bytes. El runtime sigue siendo puente C y no hay persistencia,
ACL ni concurrencia de procesos para estos archivos compartidos.
