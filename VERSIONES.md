# Versiones de NXK

Cada cambio integrado queda en un commit con autor, fecha, archivos y diferencias.
El historial completo esta en https://github.com/sevillafernan347-hue/NXK/commits/main/.
Las etiquetas identifican entregas concretas; los commits intermedios siguen
existiendo aunque no reciban una etiqueta propia.

| Etiqueta | Entrega |
| --- | --- |
| v0.0.1 | Snapshot 358156c: tareas, comunicacion, resultados, ciclos y metricas |
| v0.0.2 | Version visible unificada y resumen global del planificador |
| v0.0.3 | Semaforos kernel y arranque por bloques de hasta 120 KiB |
| v0.0.4 | Mutex con propietario, recuperacion y ciclos combinados |
| v0.0.5 | Reservas atomicas de varios permisos de semaforo |
| v0.0.6 | Mapas BIOS mas robustos y ajuste de archivos RAM |
| v0.0.7 | Persistencia NXPD1 con dos bancos, ATA escritura y restauracion RAM |
| v0.0.8 | Segmentos de usuario seguros, tablas estables e inventario PCI |
| v0.0.9 | Busqueda PCI por clase para preparar controladores |
| v0.0.10 | Cuatro espacios privados y retorno de paginas para futuros procesos |
| v0.0.11 | Copias acotadas de datos en espacios privados |

La serie de etiquetas se inicia el 3 de noviembre de 2026. v0.0.1 identifica
la entrega previa ya probada; no cambia sus archivos ni fecha del commit.
Los prototipos anteriores mostraban el texto fijo NXK 0.1. Desde v0.0.2 el
numero visible procede de lib/version.h y coincide con la etiqueta publicada.
Las versiones son experimentales y no representan porcentaje de Windows.

## Consultar o recuperar

- Historial: https://github.com/sevillafernan347-hue/NXK/commits/main/
- Etiquetas: https://github.com/sevillafernan347-hue/NXK/tags
- Entregas: https://github.com/sevillafernan347-hue/NXK/releases
- Diferencias: https://github.com/sevillafernan347-hue/NXK/compare/v0.0.1...v0.0.2

En una etiqueta o release, GitHub permite descargar ZIP/TAR del codigo fuente.
Las herramientas externas y los discos generados no se incluyen. Construir
requiere preparar las herramientas indicadas en HERRAMIENTAS.md. El historial
Git completo se obtiene clonando el repositorio; el ZIP es una instantanea.

## Publicar las siguientes entregas

1. Continuar el codigo sin reescribir commits antiguos.
2. Incrementar el numero en lib/version.h para la siguiente entrega publicada.
3. Registrar cambios, pruebas y limites en CAMBIOS.md y ESTADO.md.
4. Ejecutar las pruebas necesarias y guardar los cambios en un commit.
5. Crear una etiqueta anotada nueva sobre ese commit y subirla a GitHub.
6. Publicar notas experimentales en Releases, conservando las entregas anteriores.

No mover una etiqueta publicada ni forzar el historial para ocultar cambios.
Un arreglo posterior lleva otro commit y, si se publica como entrega, otro numero.
