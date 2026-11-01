# Primera prueba de privilegios x86

GDT amplia los segmentos existentes con codigo DPL3 (selector 43) y datos
DPL3 (selector 51), manteniendo los indices de ambas TSS. TSS normal recibe
SS0/ESP0 para que la CPU use una pila del kernel al cambiar de privilegio.
Dos paginas del heap son accesibles al usuario en una prueba aislada: codigo
solo lectura y pila escribible. El resto de PTE conserva supervisor; PDE permite
usuario solo para resolver esas entradas. Se recarga CR3 para invalidar TLB.
IRET transfiere a CPL3 con IOPL0 e IF apagada. INT 0x80 tiene DPL3 y entra en
el manejador del kernel. Se comprueba CS guardado con RPL3 y CS actual ring0.

Pruebas separadas: 414 INT 0x80 autorizado; 514 CLI rechazada con #GP(0);
614 escritura a 0x10000 rechazada con #PF(7), CR2 correcto y CS usuario.
Se conservan los siete casos anteriores de excepciones. Las pruebas terminan
en el manejador fatal y dispositivo de salida QEMU; no vuelven al programa.

Limitaciones: solo infraestructura y prueba, no procesos utilizables. Comparte
CR3 con kernel, no hay cambio de espacios ni validacion de llamadas publicas,
no carga NXE como usuario, no preserva/restaura mapeos al terminar (es una
imagen de prueba que finaliza). IRQ deshabilitadas en usuario porque las entradas
actuales aun no normalizan segmentos para ese escenario. No hay preempcion,
NX, aislamiento entre usuarios ni recuperacion del fallo. Las apps habituales
siguen en ring0. Implementacion C/asm puente pendiente de NEXTRY ASM.

Validacion: diez escenarios de excepciones/transiciones, matriz x86 32/64/128/1024 MiB y regresion x64 64 MiB superados.

## Demostracion con retorno al llamador

Comando usuario: programa conocido CPL3 carga EAX=42 y ejecuta INT 0x80.
El puente temporal kernel/usuario32.S guarda pila, registros preservados por
ABI y EFLAGS del llamador; la puerta usa SS0/ESP0 y restaura segmentos kernel,
pila del llamador y estado de interrupciones. La funcion valida codigo de salida
y CS usuario, retira permisos de usuario y restaura puerta y TSS anteriores.
Una reserva de 16 KiB contiene codigo, pila usuario, alineacion y pila kernel;
se reutiliza, sin asignaciones parciales. Las aplicaciones NXE siguen ring0.

Pruebas: retorno/repeticion sin crecimiento del heap en matriz x86 y arranque
x64. Consola por serie: usuario dos veces seguido de help, resultado correcto.
Se repiten diez escenarios anteriores de excepciones/proteccion.
El soporte es sincronico, una ejecucion, solo programa conocido y sin IRQ en
CPL3. No es una API para ejecutar codigo externo ni un proceso independiente.
Un bucle de usuario sin salida bloquearia el sistema. Fallos de usuario siguen
siendo fatales. No hay preempcion ni cambio de CR3 por proceso. GNU asm es un
puente pendiente de migrar a NEXTRY ASM.

Enlazador: limite de imagen permanece en 32 KiB; BSS NOLOAD permite ocupar
hasta 64 KiB de RAM total del kernel, dentro de la reserva fisica existente.
Se mantienen ambas comprobaciones y la validacion de longitud de build.ps1.

## Contencion de fallos en la demostracion sincronica

Solo durante la ejecucion conocida, CPL3 y vectores 0/6/13/14 se devuelve un
resultado de fallo al llamador por la pila guardada. Se registra vector, error
y CR2; la limpieza comun restaura PTE, puerta y TSS. Excepciones en ring0,
NMI, doble fallo y otros vectores conservan el tratamiento fatal.
Comando usuario fallos: cuatro programas conocidos provocan CLI, escritura
supervisor, UD2 y lectura nula. Cada caso verifica vector/error/direccion y
ejecuta despues el programa correcto; no consume mas heap tras inicializar.
Prueba de consola automatizada en lenguaje/pruebas_consola.py.
Esto recupera la demostracion; todavia no gestiona procesos independientes ni
acepta binarios externos arbitrarios. Sigue sin preempcion y con IRQ apagadas
en CPL3. Los manejadores son puentes C/asm.
