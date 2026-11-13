# Distribucion fisica x86 de NXK

Revisada para v0.0.26. Limites de region, no medicion de consumo del sistema.
El extremo derecho de cada intervalo es exclusivo.

| Region | Inicio | Fin | Uso |
| --- | --- | --- | --- |
| Mapa BIOS E820 | 0x5000 | 0x5290 | Cabecera y hasta32 registros |
| Sector BIOS | 0x7c00 | 0x7e00 | Cargador NEXTRY ASM |
| Archivo kernel | 0x10000 | 0x2e000 | Maximo120KiB leidos en32+32+56KiB |
| Imagen y variables kernel | 0x10000 | 0x40000 | Reserva192KiB; linker exige fin menor |
| Pila inicial | 0x80000 | 0x90000 | ESP inicial90000; intervalo aceptado por puente PE |
| VGA grafico optativo | 0xa0000 | 0xafa00 | Framebuffer320x200, solo supervisor |
| Heap de laboratorio | 0x100000 | 0x180000 | Incremental512KiB, pilas y buffers kernel |
| Aplicacion NXE1 | 0x180000 | 0x1a0000 | Laboratorio ring0, hasta128KiB |
| Directorio/tablas kernel | 0x200000 | 0x301000 | Identidad supervisor hasta1GiB |
| PE32 de laboratorio | 0x400000 | 0x420000 | Hasta128KiB en mapa kernel |
| Metadatos de paginas | 0x600000 | 0x610000 | Dos mapas de bits, RAM verificada |
| Paginas asignables | 0x800000 | hasta0x40000000 | Solo RAM util E820, limite1GiB |

La reserva de variables hasta0x40000 no se lee del disco: la entrada32 limpia
__bss_start..__bss_end. El linker impide exceder tanto el archivo120KiB como la
reserva192KiB. Los primeros8MiB estan excluidos del asignador fisico. La zona
adicional0x30000..0x40000 no tiene otro consumidor en el codigo actual; no se
amplia el cargador BIOS ni se cambian las direcciones de aplicaciones.

Cada proceso usa otro CR3: codigo virtual400000 RO, pila401000 RW y hasta
cuatro paginas de datos402000..406000, con respaldos fisicos propios desde8MiB.
Los datos pueden ser RO/RW; el resto de alias permanece supervisor. Su direccion
virtual400000 no comparte el PE del mapa kernel. No hay NX en x86 sin PAE.

Este mapa describe el prototipo actual. El arranque BIOS aun presupone memoria
convencional suficiente; las pruebas QEMU no certifican un equipo fisico. La
variante x64 comparte este linker, con ruta de arranque separada y limite de
archivo32KiB adicional en build.ps1.
