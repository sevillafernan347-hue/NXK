# Trabajo restante del kernel

Estado tras v0.0.12. Son hitos por implementar y verificar, no porcentajes.

| Area | Existe | Siguiente evidencia necesaria |
| --- | --- | --- |
| Procesos | Hasta cuatro programas CPL3 retenidos y turnos sincronicos, tareas kernel cooperativas | Conservar EIP/registros por proceso, estados, recursos y reanudacion planificada |
| Planificacion | Ocho tareas, esperas, mensajes, semaforos y mutex | Preempcion controlada, secciones criticas y pruebas de carreras |
| Memoria | Paginas fisicas, mapa BIOS, guardas y pool de cuatro espacios privados | Recursos de proceso, mas mapas y agotamiento aislado |
| Archivos | RAMFS, NXFS lector, snapshot NXPD1 persistente | Archivos grandes, directorios, montaje y recuperacion mas amplios |
| Hardware | BIOS, VGA texto, teclado basico, PIT, ATA PIO e inventario PCI | Enumeracion PCI, controladores USB/red/audio/graficos y pruebas por dispositivo |
| NEXTRY | Compilador a C y apps protegidas iniciales | Parametros/tipos/estructuras/punteros y migracion gradual de puentes C |
| Compatibilidad | NXE propio y demo limitada Win32 | ABI estable y APIs documentadas; compatibilidad Windows general pendiente |
| Calidad | Pruebas QEMU y versiones Git | Pruebas fisicas ASUS, cargas prolongadas, fuzzing y revision de seguridad |

El siguiente gran bloque propuesto es procesos CPL3 simultaneos. Requiere separar
memoria, contexto de CPU, recursos y terminacion antes de activar preempcion.
Cada entrega debe incluir pruebas negativas y conservar aplicaciones anteriores.
No se habilita una capacidad solo por agregar su nombre al listado.
