# Restriccion de archivos por proceso

Base:1b4726b (v0.0.21). Contrato aprobado por integracion para v0.0.22.

Cada proceso gestionado empieza sin filtro de nombres. Puede restringirse a
una lista de cero a cuatro nombres de archivo. Una vez activo, solo se admite
una lista que sea subconjunto de la anterior; el orden no importa. Cero nombres
prohibe todo acceso a archivos y no puede revertirse en ese ID. Los permisos
generales de salida/lectura/modificacion/envio se conservan como otra condicion.

Cada nombre tiene entre1 y23 caracteres del conjunto a-z,0-9,_,-,. y terminador
nulo en24bytes. No se admiten duplicados. La validacion completa precede a
cualquier cambio de politica; rechazar conserva la lista anterior.

INT81/EAX20 recibe EBX=tabla contigua de nombres de24bytes y ECX=cantidad0..4.
Se valida el rango completo de la tabla antes de leerla. Para cantidad0 no se
accede al puntero. EAX0 exito,-1 contrato,-5 intento de ampliar una restriccion.
Solo procesos gestionados; las rutas antiguas devuelven servicio no soportado.
Cabecera NXE2 ABI1 y los cuatro bits de permisos anteriores no cambian.

Los servicios2..8 validan permisos generales antes de leer punteros; despues
copian los nombres a memoria kernel y comprueban el filtro antes de copiar
datos o consultar/modificar el FS. Renombrar y copiar requieren autorizar ambos
nombres. Un nombre valido excluido devuelve-5; errores de contrato devuelven-1.
Salida de texto, mensajes, espera y memoria no dependen del filtro de archivos.

La API kernel permite reducir la lista de un proceso inactivo no terminal.
La variante interna solo modifica el proceso que ejecuta. Cancelar/terminar
conserva el filtro para diagnostico; una generacion nueva empieza sin filtro.
proceso listar muestra archivos_restringidos y archivos_permitidos; en un
proceso sin filtro, el contador0 no significa que no tenga acceso.

NEXTRY agrega la orden limitar_archivos con0..4 expresiones de texto separadas
por comas. Sin argumentos retira todo acceso. El runtime falla explicitamente
si el kernel rechaza la restriccion o la ruta no ofrece soporte.

## Alcance

Puentes C en gestion y servicios; fuente de ejemplo en NEXTRY. Restriccion por
nombre exacto en el espacio compartido RAMFS/NXFS, no propietarios, usuarios,
directorios ni ACL por objeto. Renombrar o recrear el mismo nombre puede cambiar
el contenido accesible. No borra archivos al terminar ni revoca bytes ya leidos.
Kernel y rutas antiguas sin PCB conservan su autoridad anterior. Se mantienen
cuatro procesos, una CPU y los limites de archivos y de arranque.

## Verificacion prevista

Listas de0/4, subconjuntos, reordenamiento, duplicados, nombres invalidos,
tablas cruzadas/huecos, rechazo sin efectos, ambas rutas de copiar/renombrar,
interseccion con permisos globales, IDs nuevos, ejemplo real y regresiones.
Registrar resultados solo despues de ejecutar las pruebas de integracion.

## Resultados de integracion

80 pruebas del compilador superadas:24 generales y56 de extensiones.
test.ps1 x86 -MemoryMatrix superado con32,64,128 y1024MiB; regresion x64
con64MiB superada. Imagen x86 normal73740B/test102412B; x64 normal24584B/
test28680B. Los limites BIOS y el numero de procesos no cambian.

pruebas_archivos_procesos.py superada: listas0..4, reordenamiento, subconjuntos,
idempotencia, intentos de ampliacion, nombres invalidos y duplicados. Tabla
de solo lectura, cruces, huecos y validacion completa incluso si un NUL aparece
antes de la pagina ausente. Basura tras NUL ignorada como indica el contrato.
Servicios2..8, ambos extremos de copia/renombre, coincidencia exacta y lectura
NXFS, permisos generales y stdout independiente. Dos procesos con politicas
distintas; cancelacion, reciclaje de IDs, ejemplo NEXTRY real y salida42.
Heap/paginas estables y discos originales intactos usando snapshot.

Regresiones de buffers privados, permisos generales, dependencias y resumen
de procesos y22 escenarios NXE2 desde disco superadas. La revision independiente del codigo no encontro
bloqueadores en las comprobaciones de nombres, listas o vida del registro.

Los416B adicionales de PCB dejaban la imagen de pruebas88B fuera de la reserva.
El registro de orden de rondas (96B) se presta desde la pila del llamador de
prueba mientras corren sus ocho tareas y se retira al liberarlas. Esto evita
reservarlo permanentemente, conserva las aserciones y no crece las pilas de
las tareas. La matriz completa paso con esa integracion.

## Uso NEXTRY

```text
limitar_archivos "propio", "copia"
# Las dos operaciones siguen sujetas a los permisos generales.
copiar_archivo "propio", "copia"
limitar_archivos "propio"
# Sin argumentos retira tambien propio.
limitar_archivos
```

La fuente completa esta en lenguaje/ejemplos/proceso_archivos.nxf. Se compila
con compilar-usuario.ps1 -Salida, se incluye en NXFS y se ejecuta mediante
proceso cargar / proceso ejecutar. El operador puede inspeccionar los archivos
despues; el filtro limita a la aplicacion, no la consola confiable del kernel.
