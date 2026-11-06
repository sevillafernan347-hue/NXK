# NXK v0.0.8: aislamiento de segmentos, tablas y deteccion PCI

Base v0.0.7, commit ccb6400. Kernel x86 BIOS de una CPU.

## Fallo corregido en aplicaciones

Una aplicacion CPL3 podia dejar DS o ES con selector nulo, legal en x86.
Al ejecutar INT80 o provocar UD2, el stub entraba al kernel y accedia a
variables con ese segmento nulo: el kernel podia caer por #GP en ring0.
NEXTRY ASM ahora normaliza DS/ES/FS/GS tras guardar registros del marco de
excepcion y antes de llamar C. La salida INT80 restaura DS antes de escribir
el selector guardado. Se preservan EAX y offsets del marco, incluidos fallos
previos. Seis casos NXE2 ejecutan DS0, ES0 y todos nulos, cada uno con salida
y UD2: la consola recupera control y puede lanzar otra app despues.

## Tablas de paginacion

La validacion previa acepta una union continua de regiones RAM que cubra
0x200000..0x301000 aun si el BIOS la divide dentro de paginas, sin escribir el
mapa. Una reserva superpuesta o un hueco minimo rechaza la configuracion.
Intentar `nxk_paginacion_iniciar` tras activarla devuelve -2 sin recrear tablas,
perder guardas ni tocar CR0/CR3. Pruebas internas usan mapas desordenados,
solapamientos, huecos y overflow, y comparan el contenido completo de las
tablas antes/despues, ignorando bits accessed/dirty que modifica el procesador.

## Inventario PCI

`pci` enumera bus0..255, ranuras0..31 y funciones adicionales solo cuando la
cabecera del dispositivo indica multifuncion. Lee IDs, clase/subclase/interfaz,
revision e IRQ por CF8/CFC (mecanismo 1), preserva selector y IF, y devuelve
hasta 64 registros en tabla estatica. El resumen cuenta detectados, guardados
y omitidos. No asigna heap, no escribe configuracion, no habilita DMA ni maneja
interrupciones de los dispositivos. `lib/pci.h` expone consulta por indice y
lectura DWORD validada para trabajo futuro de controladores.

Pruebas QEMU comparan inventario con QMP tanto en maquina basica como con
puente secundario y funciones discontinuas; ejecutar tres veces mantiene
recursos PCI y heap, y luego corre NEXTRY. La prueba interna simula 70
funciones para comprobar truncamiento seguro a64, reinicio del inventario,
contratos invalidos y preservacion del selector/IF.

Esta enumeracion usa configuracion PCI legado; algunos equipos PCIe precisan
ECAM/ACPI y controladores que todavia no existen. Ni inventario ni pruebas QEMU
certifican la ASUS fisica. Los nuevos puentes C se migraran cuando NEXTRY
tenga estructuras, punteros y acceso seguro a E/S.

Referencia de mecanismo 1: https://www.intel.com/Assets/PDF/datasheet/316966.pdf
