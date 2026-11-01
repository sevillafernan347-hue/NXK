# Archivos RAM desde NEXTRY CPL3

Ordenes nuevas: guardar_archivo "nombre", "texto"; mostrar_archivo "nombre";
borrar_archivo "nombre". El compilador exige argumentos texto. Runtime usuario
usa INT 0x81: servicio 2 escribir, 3 leer y 4 borrar. EBX nombre terminado en
cero (maximo 23 caracteres), ECX buffer y EDX longitud/capacidad hasta 256.
EAX devuelve 0 para escribir/borrar, cantidad leida o FFFFFFFF si hay error.

El kernel copia el nombre y los datos antes de escribir. Para leer exige un
destino dentro de la pagina escribible de pila de usuario; rechaza codigo,
memoria kernel y rangos que cruzan el final. No dereferencia entradas sin
validar rango. Los archivos son compartidos con la consola, ocho entradas y
volatiles; no hay permisos por proceso, directorios ni almacenamiento en disco.
Un programa puede reemplazar/borrar archivos RAM por nombre. No es un sandbox
con permisos de archivos ni plataforma para ejecutar fuentes desconocidas.

El ejemplo NEXTRY crea nota, muestra el contenido y la borra. Casos raw verifican
lectura correcta, destino codigo rechazado, destino kernel rechazado, nombre
sin terminador y cruce fuera de memoria usuario. Fixture usr_test solo se crea
si no existe y se borra al terminar la prueba; no sobrescribe archivos previos.

20 pruebas del compilador, matriz x86, consola y regresion x64 superadas.
ABI NXE 1 anterior no ofrece archivos: su runtime falla explicitamente si se
usan estas ordenes. En modulo kernel hay puentes directos; el runtime CPL3 es
la ruta para aplicaciones protegidas. No se altero la tabla NXE 1 existente.

Fuentes NEXTRY negativas adicionales: mostrar archivo ausente y guardar nombre ../x. Ambas terminan la app con error y conservan help. Suite de cinco fuentes negativas superada y ejemplo correcto restaurado.
