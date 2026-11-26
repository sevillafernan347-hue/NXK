# Lecturas NXFS diferidas para procesos

Rama `kernel/lecturas-diferidas-v042`, base `7da28be`. Alcance: lector NXFS,
planificador CPL3, servicio de lectura por tramos, consola y pruebas.

## Contrato

El servicio existente INT 81h / 5 conserva sus registros, el máximo de
256 bytes por tramo y sus códigos. RAM mantiene prioridad. Los procesos
gestionados ceden el turno cuando un tramo con datos procede de NXFS.
Las lecturas RAM y la ruta sin gestor conservan su ejecución inmediata.

Cada proceso puede esperar un tramo. El gestor conserva su ID completo,
la revisión del volumen, el nombre y el destino validado. Atiende una
petición de disco a la vez y rota entre solicitantes. Entre turnos
procesa como máximo un sector. Si solo quedan peticiones de disco,
`proceso ejecutar` también usa su presupuesto para atenderlas.

El núcleo verifica el archivo completo antes de entregar el tramo. Una
copia verificada de hasta 4 KiB se puede usar en el siguiente tramo
contiguo del mismo proceso y archivo. El almacenamiento de esa copia es
común y se reserva al primer uso. El resultado se copia con el mapa del
kernel y se devuelve en EAX al reanudar. Si falla la petición, el destino
queda intacto. Revocar lectura o excluir el nombre devuelve -5.

Mientras espera, el destino conserva sus páginas. Cancelar, eliminar el
proceso o cambiar el montaje impide entregar datos obsoletos. El estado
`disco` aparece en `proceso resumen`. Es un puente C x86. Cada lectura ATA
individual sigue siendo síncrona y las demás aplicaciones pueden avanzar
entre sectores. La verificación iniciada desde la consola conserva su
paso manual y avanza entre turnos cuando no hay lectores esperando.

## Pruebas

- El simulador de disco comprobó ocho lecturas de sector para entregar un
  archivo de 4096 bytes en 16 tramos. También comprobó aislamiento por
  proceso, cambio de revisión y corrupción en el byte final sin modificar
  el búfer de destino.
- `lenguaje/pruebas_lectura_diferida.py` arrancó la imagen BIOS en QEMU.
  Pasaron dos lectores pendientes, el avance de un tercer proceso, la
  protección de dos páginas de destino y la lectura completa de 4096 bytes.
  También pasaron cancelación, reutilización del identificador de espacio
  con una generación nueva, revocación del permiso, exclusión del nombre
  y cambios de montaje. Para repetir, compila con `build.ps1 -Architecture x86`
  y ejecuta el script con el Python del proyecto.
- Pasaron las regresiones de procesos, los 22 escenarios de NXFS y los 10
  escenarios de lectura NXFS desde aplicaciones NEXTRY.
- `test.ps1 -Architecture x86 -MemoryMatrix` pasó con 32, 64, 128 y
  1024 MiB virtuales y 166 comprobaciones ATA simuladas. La regresión x64
  pasó con 64 MiB virtuales. La imagen de pruebas se detiene tras notificar
  el resultado a QEMU. La ASUS física sigue pendiente.
