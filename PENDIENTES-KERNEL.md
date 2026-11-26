# Trabajo restante del kernel

Estado tras v0.0.44.

| Area | Existe | Siguiente evidencia necesaria |
| --- | --- | --- |
| Procesos | Cuatro PCB CPL3, memoria privada, mensajes, resultados, permisos y espera para lecturas NXFS | Permisos por canal u objeto y más recursos por proceso |
| Planificación | Tareas kernel, preempción CPL3 y lecturas NXFS con sondeo ATA acotado entre turnos | Esperas para otros dispositivos, secciones críticas y pruebas de carreras |
| Memoria | Páginas físicas, guardas, espacios privados, permisos RO/RW y límite de ejecución por segmento en CPL3 | NX por página mediante PAE, mapas mayores, asignación por bytes y memoria compartida |
| Archivos | RAMFS, NXFS con montaje, verificación y lecturas diferidas, instantáneas NXPD1 | Archivos grandes, directorios, varios volúmenes y recuperación más amplia |
| Hardware | BIOS, VGA texto, teclado basico, PIT, ATA PIO e inventario PCI | Controladores USB/red/audio/graficos, ECAM/ACPI y pruebas por dispositivo |
| NEXTRY | Compilador a C y apps protegidas iniciales | Parametros/estructuras/punteros y migracion gradual de puentes C |
| Compatibilidad | NXE propio y demo limitada Win32 | ABI estable y APIs documentadas; compatibilidad Windows general pendiente |
| Calidad | Pruebas QEMU, simulacion de estados ATA y versiones Git | Pruebas fisicas ASUS, cargas prolongadas, fuzzing y revision de seguridad |

Existe un filtro saliente por IDs completos; faltan canales con identidad y
permisos independientes de los procesos. MINI NXK tiene paneles graficos fijos
y terminal con teclado; faltan raton, ventanas y aplicaciones graficas.

ATA valida inicio y cierre de lectura/IDENTIFY. Sus plazos usan el reloj PIT
y el límite de sondeos sigue activo. `discos estado` consulta la fase y el
límite alcanzado sin acceder al disco. El canal primario se puede reiniciar
manualmente. Las lecturas NXFS gestionadas avanzan por sondeos acotados.
La transferencia de 256 palabras y las operaciones de consola siguen
siendo síncronas. Quedan pendientes IRQ, DMA y AHCI. Un fallo al escribir
no revierte bytes enviados al disco.

El siguiente bloque propuesto es espera de dispositivos, recursos por proceso
y permisos por canal/objeto. Los permisos generales y filtros por nombre ya existen. IPC de valores uint32 ya espera espacio al enviar y permite
observar resultados; faltan canales con permisos y transferencia de datos
mayores. El detector cubre dependencias explicitas al registrarlas y despacharlas,
no todos los bloqueos de un programa. La preempcion CPL3 ya existe; el kernel sigue
cooperativo.
Cada entrega debe incluir pruebas negativas y conservar aplicaciones anteriores.
No se habilita una capacidad solo por agregar su nombre al listado.
