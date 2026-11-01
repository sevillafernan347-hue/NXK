# Directorio de paginas privado para la demostracion de usuario

La demostracion usa un CR3 diferente al kernel. Reserva unica de 28 KiB en
heap: codigo, pila usuario, pila kernel, dos paginas para directorio/tabla y
margen de alineacion. Copia directorio y tabla inferior cada vez; solo la copia
concede permiso usuario a codigo (lectura) y pila (escritura). Los mapas altos
se comparten como supervisor. No se modifican permisos de la tabla kernel.

Al volver, lee CR3 para verificar el espacio utilizado, restaura CR3 kernel,
revoca permisos de la copia y restaura puerta INT/TSS. Verifica que marcos y
permisos principales no cambiaron, ignorando solo bits A/D mantenidos por CPU.
Las paginas que contienen las tablas privadas siguen siendo supervisor.

Prueba nueva: programa usuario intenta escribir en su propio directorio de
paginas; se exige #PF(7) con CR2 igual a esa direccion. usuario fallos ahora
verifica cinco casos y vuelve a ejecutar correctamente tras cada uno.
Matriz x86 32/64/128/1024 MiB, regresion x64 y consola automatizada.

Limites: un unico espacio de demostracion reutilizable; no hay multiples
procesos ni asignaciones virtuales distintas por programa. El mapa sigue siendo
identidad, sin NX. No carga NXE como usuario. IRQ deshabilitadas durante CPL3,
sin preempcion. Las funciones de administracion siguen siendo puente C; el
cambio de privilegio y retorno se mantienen en NEXTRY ASM.

Extension: sexto caso deja ESP usuario en 4 y ejecuta PUSH hacia pagina cero.
Se exige #PF(6), CR2=0 y ESP guardado=4; la entrada usa pila SS0/ESP0 valida
y vuelve a consola. El historial incluye ESP de usuario en los registros de
fallo y limpia ese campo al terminar correctamente. usuario fallos verifica
seis casos. No implica recuperacion general de pilas del kernel.

Politica explicita: al copiar directorio/tabla se retira U/S de todas las
entradas heredadas antes de habilitar solo codigo y pila propios. Evita heredar
permisos de usuario accidentales si se amplian los mapas kernel en el futuro.
La demostracion normal ahora apila y recupera 42 desde su pila para verificar
el acceso permitido. Nuevos casos: escritura sobre codigo propio (#PF7 y CR2
exacto) y lectura de heap supervisor en 0x100000 (#PF5). usuario fallos ejecuta
ocho casos y recupera el programa correcto tras cada uno. Matriz x86, consola
y regresion x64 superadas. No se afirma proteccion NX: en modo actual las
paginas escribibles tambien pueden ejecutarse. No hay usuarios multiples.
