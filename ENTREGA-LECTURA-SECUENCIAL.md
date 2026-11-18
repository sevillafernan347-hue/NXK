# Lectura secuencial de NXFS en modo usuario

Base 9478a2a (v0.0.33). La integracion aprueba una consulta interna de revision
del disco esclavo y la reutilizacion acotada del buffer NXFS existente.
Alcance: ATA, NXFS, pruebas y documentacion. La ABI de usuario y el formato
en disco se mantienen.

Tras verificar todo el archivo, el lector por rangos puede reutilizar sus
bytes para el siguiente tramo contiguo del mismo archivo. Desde el byte cero
se verifica de nuevo. Cambiar de archivo, saltar tramos, remontar, mostrar
desde consola o modificar/reidentificar el esclavo obliga a volver a leer.
El buffer nunca se publica si la lectura o la verificacion fallan.

En v0.0.33, una aplicacion NEXTRY podia agotar su turno antes de mostrar un
archivo de 4096 bytes. Pedia 16 tramos de 256 bytes y el kernel verificaba
los ocho sectores completos para cada tramo. El lector secuencial necesita
ocho lecturas en total para ese archivo cuando la secuencia es continua.
La copia se descarta al completar el archivo. Las lecturas por rangos
siguen respetando capacidad, permisos y lista de nombres por proceso.

El cambio reutiliza el buffer existente de 4096 bytes. Agrega metadatos
estaticos por una sola secuencia, no por proceso. Cualquier verificacion
nueva reemplaza la copia. La revision del esclavo se incrementa antes de
intentar IDENTIFY o WRITE e invalida la copia incluso si falla el comando.
La revision deja de habilitar la reutilizacion al saturarse. El modelo
corresponde a un canal IDE sin intercambio de medios en caliente.

Una repeticion mostro que la salida por consola tambien puede superar
los diez ticks originales. La integracion aprueba 100 ticks (aproximadamente
un segundo) para la ejecucion sincronica de `usuario nextry`. El PIT sigue
deteniendo un bucle. Los turnos de procesos gestionados siguen en diez ticks.
El lector comparte bytes verificados mientras los tramos son contiguos y la
revision coincide. El protocolo ATA, formato NXFS1 y runtime se conservan.

## Comprobaciones

- `test.ps1 -Architecture x86 -MemoryMatrix` paso en QEMU con 32, 64, 128
  y 1024 MiB. La regresion x64 paso con 64 MiB.
- La suite del controlador paso 141 comprobaciones, incluida la invalidacion
  al fallar WRITE despues de transferir datos y la saturacion de la revision.
- La suite host usa el lector real y un disco simulado. Verifico 16 tramos
  de 256 bytes con ocho lecturas, reinicio al saltar tramos, cambios de
  archivo y revision, remontaje y destino intacto ante huella incorrecta.
- Pasaron diez casos con aplicaciones NEXTRY en CPL3. Un archivo de 4096
  bytes se mostro exacto y la corrupcion del ultimo byte se rechazo antes
  de emitir el primer tramo. Los discos de prueba se conservaron.
- Pasaron 18 escenarios de NXFS y 38 comprobaciones de persistencia.
- `lenguaje/pruebas_usuario_nxf.py --caso bucle` confirmo que el PIT
  interrumpe un bucle de NEXTRY y que la consola vuelve a responder.
