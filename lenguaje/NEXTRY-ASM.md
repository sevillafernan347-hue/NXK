# NEXTRY ASM 0.1

Ensamblador propio de dos pasadas. Lee fuentes `.nxa`, resuelve etiquetas y expresiones de suma/resta y genera bytes de maquina directamente. No invoca otro ensamblador. La implementacion inicial del ensamblador esta escrita en Python; su autoalojamiento en NEXTRY sigue pendiente.

El arranque principal de 32 bits ya usa `boot/arranque32.nxa` mediante `build.ps1`. El archivo `boot32.S` se conserva como referencia anterior; ya no construye ese sector de arranque. El resto del nucleo conserva dependencias de C y ensamblador externo, incluida la entrada al codigo C. La variante de 64 bits todavia usa su cadena anterior.

## Instrucciones disponibles

`desactivar_interrupciones`, `activar_interrupciones`, `direccion_adelante`, `detener`, `cero ax`, `segmento ds/es/ss/fs/gs, ax`, `leer_control eax, cr0`, `escribir_control cr0, eax`, `saltar_registro eax`.

`cargar8 registro, valor`, `cargar16 registro, valor`, `cargar32 registro, valor`, `interrupcion numero`, `entrada8 puerto`, `salida8 puerto`, `unir8 valor`, `unir32 valor`, `cargar_gdt etiqueta`, `saltar_lejos selector, etiqueta`, `saltar etiqueta`, `si_error etiqueta`.

Las formas de operandos estan limitadas a las implementadas en `ensamblador.py`; no se admite todavia todo el repertorio x86. Entrada/salida usan AL y un puerto inmediato de 8 bits; unir usa AL o EAX. Los saltos cercanos se codifican segun el modo activo.

## Directivas

- `origen direccion`: direccion virtual inicial; no rellena el archivo.
- `modo 16` o `modo 32`: tamano de operandos por defecto.
- `etiqueta:`: nombre de una posicion.
- `bytes`, `palabra`, `doble`, `cuadruple`: datos de 1, 2, 4 u 8 bytes, little-endian, separados por comas.
- `alinear cantidad`: relleno a la siguiente alineacion de archivo.
- `rellenar_hasta posicion`: relleno con ceros hasta un desplazamiento de archivo.
- `#`: comentario hasta el final de la linea.

## Migracion siguiente

Ya se incorporo `kernel/excepciones32.nxa` a la construccion. Nuevas instrucciones implementadas: `guardar_registros32` (PUSHAD), `alinear_pila16` (AND ESP, -16) y `llamar_registro eax` (CALL EAX). Esta entrada fatal no retorna: detiene la CPU despues del diagnostico. El puente C aun configura la IDT y enlaza el destino de llamada.

Ampliar instrucciones y operandos con pruebas de codificacion, sustituir `entry32.S`, implementar salida de objetos/enlace y generar instrucciones desde el compilador NEXTRY. Despues migrar componentes C y la variante de 64 bits. Tener este ensamblador no significa que todo NXK ya este escrito en NEXTRY.
# Copia de memoria

`copiar_bytes` emite REP MOVSB. En modo 32 usa ESI como origen, EDI como
destino y ECX como cantidad. Modifica esos tres registros. La direccion
depende de DF; usar `direccion_adelante` antes de una copia hacia adelante.
Ejemplo completo: `lib/copiar32.nxa`.


## Puente de modo usuario x86

Los archivos kernel/usuario_entrar.nxa, usuario_salir.nxa y usuario_abortar.nxa
sustituyen el puente GNU asm. Instrucciones nuevas, solo modo 32:

- guardar_banderas32 / recuperar_banderas32: PUSHFD / POPFD.
- apilar_inmediato32 valor: PUSH de un inmediato de 32 bits.
- leer_memoria32 registro, direccion: MOV desde direccion absoluta DS.
- escribir_memoria32 direccion, registro: MOV hacia direccion absoluta DS.

Las referencias externas se declaran con externo nombre. El enlace provisional
obtiene las direcciones y el ensamblado final las resuelve; se comprueba que
ninguna cambie al volver a enlazar. El envoltorio .S generado solo empaqueta
los bytes NXA mediante incbin, no contiene la implementacion del puente.
Se mantienen los convenios cdecl y los selectores existentes. Estos intrinsecos
son privilegiados y no convierten NEXTRY ASM en una plataforma segura por si solos.
