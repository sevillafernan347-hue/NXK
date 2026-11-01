# Llenado de memoria x86 en NEXTRY ASM

La rutina memset de 32 bits ahora se genera directamente desde `lib/llenar32.nxa`. Recibe destino, valor y cantidad con ABI cdecl; conserva EDI y devuelve el destino. La implementacion C se conserva solo para x64.

Se agregaron instrucciones de pila, copia entre registros y retorno a NEXTRY ASM. El asignador de memoria sigue siendo C y no se ha sustituido aun por un gestor completo de RAM.

Se comprueban relleno de bytes, truncamiento del valor a un byte, retorno del puntero, longitud cero y centinelas para detectar escritura fuera del rango.
