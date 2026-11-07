# Trabajo restante del kernel

Estado tras v0.0.15. Son hitos por implementar y verificar, no porcentajes.

| Area | Existe | Siguiente evidencia necesaria |
| --- | --- | --- |
| Procesos | Cuatro PCB CPL3 con turnos automaticos, espera temporal y memoria privada | Espera de E/S, IPC y recursos por proceso |
| Planificacion | Tareas kernel, sincronizacion y preempcion CPL3 por rondas | Espera de dispositivos e IPC, secciones criticas y pruebas de carreras |
| Memoria | Paginas fisicas, mapa BIOS, guardas y pool de cuatro espacios privados | Recursos de proceso, mas mapas y agotamiento aislado |
| Archivos | RAMFS, NXFS lector, snapshot NXPD1 persistente | Archivos grandes, directorios, montaje y recuperacion mas amplios |
| Hardware | BIOS, VGA texto, teclado basico, PIT, ATA PIO e inventario PCI | Controladores USB/red/audio/graficos, ECAM/ACPI y pruebas por dispositivo |
| NEXTRY | Compilador a C y apps protegidas iniciales | Parametros/estructuras/punteros y migracion gradual de puentes C |
| Compatibilidad | NXE propio y demo limitada Win32 | ABI estable y APIs documentadas; compatibilidad Windows general pendiente |
| Calidad | Pruebas QEMU y versiones Git | Pruebas fisicas ASUS, cargas prolongadas, fuzzing y revision de seguridad |

El siguiente bloque propuesto es espera de dispositivos e IPC por proceso, seguido por
mas recursos y permisos. La preempcion CPL3 ya existe; el kernel sigue cooperativo.
Cada entrega debe incluir pruebas negativas y conservar aplicaciones anteriores.
No se habilita una capacidad solo por agregar su nombre al listado.
