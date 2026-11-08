# Trabajo restante del kernel

Estado tras v0.0.19. Son hitos por implementar y verificar, no porcentajes.

| Area | Existe | Siguiente evidencia necesaria |
| --- | --- | --- |
| Procesos | Cuatro PCB CPL3, memoria privada, mensajes, resultados y permisos por operacion | Espera de E/S, permisos por archivo/canal y recursos por proceso |
| Planificacion | Tareas kernel, preempcion CPL3, esperas y detector inicial de ciclos | Espera de dispositivos, secciones criticas y pruebas de carreras |
| Memoria | Paginas fisicas, mapa BIOS, guardas y pool de cuatro espacios privados | Recursos de proceso, mas mapas y agotamiento aislado |
| Archivos | RAMFS, NXFS lector, snapshot NXPD1 persistente | Archivos grandes, directorios, montaje y recuperacion mas amplios |
| Hardware | BIOS, VGA texto, teclado basico, PIT, ATA PIO e inventario PCI | Controladores USB/red/audio/graficos, ECAM/ACPI y pruebas por dispositivo |
| NEXTRY | Compilador a C y apps protegidas iniciales | Parametros/estructuras/punteros y migracion gradual de puentes C |
| Compatibilidad | NXE propio y demo limitada Win32 | ABI estable y APIs documentadas; compatibilidad Windows general pendiente |
| Calidad | Pruebas QEMU y versiones Git | Pruebas fisicas ASUS, cargas prolongadas, fuzzing y revision de seguridad |

El siguiente bloque propuesto es espera de dispositivos, recursos por proceso
y permisos por archivo/canal. Los permisos generales por operacion ya existen. IPC de valores uint32 ya espera espacio al enviar y permite
observar resultados; faltan canales con permisos y transferencia de datos
mayores. El detector cubre dependencias explicitas al registrarlas y despacharlas,
no todos los bloqueos de un programa. La preempcion CPL3 ya existe; el kernel sigue
cooperativo.
Cada entrega debe incluir pruebas negativas y conservar aplicaciones anteriores.
No se habilita una capacidad solo por agregar su nombre al listado.
