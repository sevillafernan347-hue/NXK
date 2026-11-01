# Formato ejecutable NXE 1

NXE es el formato nativo propio de NXK. Cambiar la extensión de un archivo EXE a NXE no lo convierte. Los fuentes en español usan `.nxf`.

## Cabecera de 64 bytes

Todos los números se guardan little-endian.

| Posición | Tamaño | Campo |
|---|---|---|
| 0 | 4 | Firma: bytes `4E 58 45 00` (`NXE` seguido de cero). |
| 4 | 2 | Versión: 1. |
| 6 | 2 | Arquitectura: 1 = x86 de 32 bits; 2 = x86-64. |
| 8 | 4 | Tamaño de cabecera: 64. |
| 12 | 4 | Dirección de carga: `0x180000` en esta versión. |
| 16 | 4 | Desplazamiento del punto de entrada dentro de la carga. |
| 20 | 4 | Tamaño de los bytes de código/datos que siguen a la cabecera. |
| 24 | 4 | Memoria necesaria, incluyendo las variables inicializadas a cero. Máximo 128 KiB. |
| 28 | 4 | FNV-1a de 32 bits de los bytes de la carga. |
| 32 | 32 | Reservados; deben ser cero. |

Después de la cabecera se encuentran los bytes de código y datos. El cargador verifica arquitectura, tamaños, dirección, campos reservados y suma antes de copiar y ejecutar. Inicializa a cero la memoria adicional.

FNV-1a detecta corrupción accidental. No es una firma digital ni prueba quién creó un archivo.

## ABI nativa

La entrada recibe un puntero a la tabla `NXE_API` declarada en `lib/nxe.h` y devuelve un código entero. La tabla contiene versión, tamaño y operaciones del kernel. Los punteros y el convenio de llamadas corresponden a la arquitectura: cdecl para x86 y ABI System V para x86-64, según el destino freestanding del compilador.

Cada binario es específico de su arquitectura. El runtime comprueba la versión y el tamaño de la tabla. El ejecutable retorna al núcleo al terminar normalmente. En esta versión `detener` o un fallo detienen la CPU.

La región NXE empieza justo al terminar el heap fijo de laboratorio. Solo puede cargarse una aplicación allí a la vez. No hay relocalización, memoria virtual por proceso, cargador de bibliotecas, permisos de páginas o firmas. Esos cambios requerirán ampliar el formato y su versión.
