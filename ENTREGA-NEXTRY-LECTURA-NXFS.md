# Lectura NXFS desde aplicaciones NEXTRY protegidas

La resolucion visible de lectura busca primero en RAM; solo el error de ausencia
(-2) consulta NXFS montado. No se sustituye un error de nombre/capacidad por otro
backend. Un archivo RAM vacio sigue existiendo y oculta el de disco. Borrar la
copia RAM permite volver a ver la copia NXFS, sin modificar el disco.

mostrar_archivo usa esta resolucion mediante el servicio 3, conservando nombre
copiado/validado y destino solo en pagina escribible del usuario. El volumen
verifica FNV completo antes de copiar datos. La lectura de usuario se mantiene
en 256 bytes maximo; archivos mayores fallan sin publicar contenido parcial.
El comando leer de consola y el puente NEXTRY kernel usan la misma prioridad.
archivos lista solo RAM; volumen listar lista disco. volumen leer admite 4096.

Ejemplo: build.ps1 -FuenteUsuario lenguaje/ejemplos/usuario_volumen.nxf
Luego run.ps1 -Volumen build/x86/datos-nxfs.img; en consola volumen montar y
usuario nextry. No cambia la ABI NXE 1 anterior. Guardar/borrar archivos desde
NEXTRY continua afectando solo RAM, nunca el volumen. Sin ACL por proceso.
El disco es imagen virtual IDE secundaria, con snapshot; no hay ATA escritura.

Pruebas especificas en lenguaje/pruebas_usuario_volumen.py: lectura CPL3 desde
disco, prioridad RAM, RAM vacia sin fallback, retorno a disco tras borrar RAM,
checksum de contenido corrupto, 257 bytes con capacidad usuario de 256,
archivo ausente y falta de montaje. En todos los casos consola sigue activa y
se comparan las imagenes base para comprobar que no cambiaron. El script
restaura la aplicacion por defecto. Matriz x86, consola general y x64 superadas.

Limites: lectura sincronica, una app, sin procesos multiples ni cargador de
programas desde NXFS. Sondeos ATA acotados pueden retrasar IRQ durante servicio;
no hay garantia de tiempo real. FNV no autentica contenido. Puentes C pendientes
de migracion NEXTRY; apps se compilan desde nuestro lenguaje.
