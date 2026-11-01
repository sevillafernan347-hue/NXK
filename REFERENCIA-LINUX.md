# Linux como referencia para NXK

Archivo aportado: linux-7.3.1.tar.xz, 162406920 bytes.
SHA256: fa595aad93734f829c963c2a90ef7c3673224fd7fdae7b49406e397cde2de231
Nombre del paquete no verificado contra una publicacion oficial.
Se inventariaron los 95979 archivos regulares, sin extraer ni ejecutar el arbol.
Se leyeron COPYING y archivos de construccion de x86, planificador, memoria y fs.
Esto no constituye una revision de todos los archivos fuente.

| Linux | Responsabilidad | Situacion y siguiente paso en NXK |
| --- | --- | --- |
| arch | CPU, arranque, interrupciones | boot y entradas NXA; aislar despues x86 |
| kernel/sched | Planificacion | tareas cooperativas; estados y finalizacion |
| mm | Memoria | paginas fisicas; pendiente memoria virtual |
| fs | Archivos | pendiente empezar por archivos en RAM |
| drivers | Dispositivos | VGA, serie y PS/2 basicos; faltan controladores |
| block | Almacenamiento | pendiente interfaz de bloques |
| net | Red | pendiente |
| include | Contratos compartidos | lib contiene cabeceras actuales |
| init | Inicio | kernel_main; separar etapas al crecer |
| tools, scripts | Construccion y utilidades | build.ps1 y herramientas Python |
| Documentation | Documentacion | guias y entregas en espanol |
| security, crypto, ipc | Seguridad y comunicacion | pendientes |
| sound, rust, virt, io_uring | Subsistemas especializados | fuera del alcance inmediato |

No se importo codigo Linux a NXK. El COPYING del paquete declara GPL-2.0
con Linux-syscall-note y remite a licencias por archivo. Gratuito no significa
sin condiciones de reutilizacion. Registrar procedencia y revisar condiciones
antes de incorporar codigo. La ruta actual sigue siendo implementacion propia.

Primera mejora propia tras esta revision: estados LISTA/EJECUTANDO/TERMINADA
en la demostracion cooperativa. Las tareas terminadas se omiten y se comprueba
que ambas alcancen ese estado. Sigue siendo puente C con cambio de pila NXA.
Pruebas x86 con 32/64/128/1024 MiB superadas; no certifican hardware fisico.

## Inventario por entrada principal

.clang-format: 1
.clippy.toml: 1
.cocciconfig: 1
.editorconfig: 1
.get_maintainer.ignore: 1
.gitattributes: 1
.gitignore: 1
.mailmap: 1
.pylintrc: 1
.rustfmt.toml: 1
COPYING: 1
CREDITS: 1
Documentation: 11438
Kbuild: 1
Kconfig: 1
LICENSES: 23
MAINTAINERS: 1
Makefile: 1
README: 1
arch: 18681
block: 103
certs: 12
crypto: 184
drivers: 37988
fs: 2354
include: 6725
init: 17
io_uring: 89
ipc: 13
kernel: 746
lib: 914
mm: 206
net: 1906
rust: 596
samples: 293
scripts: 701
security: 314
sound: 2997
tools: 9618
usr: 23
virt: 21