# Aplicacion NEXTRY compilada para modo usuario

Comando usuario nextry: ejecuta lenguaje/ejemplos/usuario.nxf compilado con un
runtime CPL3 propio. Salida texto/numeros por INT 0x81; finalizacion INT 0x80.
Soporta operaciones normales del compilador, funciones locales sin parametros,
afirmaciones, enteros, logicos y texto. No modifica la ABI NXE existente.
Es imagen binaria integrada de prueba, no un nuevo formato ejecutable publico.

compilar-usuario.ps1 compila fuente y runtime con enlace a 0x400000. Una pagina
para codigo/constantes, sin datos globales, mas una pagina de pila. El linker
rechaza imagen mayor a 4096 bytes o datos globales. Entry llama inicio y sale
con 0; error de programa sale con 1. detener termina la app, no la CPU.
memoria_usada y puertos no estan disponibles en este runtime y terminan con
error; no inventan valores ni operan hardware. Textos de hasta 255 bytes por
mostrar, salida numerica uint32. No hay entrada de teclado ni archivos usuario.

El kernel reserva una vez 32 KiB del heap, incluyendo dos tablas privadas y
directorio. Para esta app mapea sus paginas fisicas en 0x400000 y 0x401000;
las direcciones originales de heap siguen supervisor. El mapa kernel mantiene
su propio 0x400000, usado por PE32 sin colision. Retorno/fallo/limite PIT
restauran CR3 y permisos. Las apps NXE anteriores siguen en ring0.

Para fuente propia: build.ps1 -FuenteUsuario ruta.nxf. Arrancar nueva imagen
y usar usuario nextry. test.ps1 acepta la misma opcion para validacion.
No carga rutas arbitrarias desde disco o RAM y no ejecuta multiples apps.
El presupuesto temporal actual sigue en diez ticks de PIT por ejecucion.

Validacion: ejemplo imprime texto y numero 10, afirma resultado y termina en
CPL3. Matriz x86 32/64/128/1024 MiB, consola serie y regresion x64 superadas.
Las pruebas existentes de servicios, fallos y recuperacion siguen activas.
x64 no ofrece esta app CPL3; conserva su variante anterior. Runtime C y linker
son puentes de compilacion pendientes de migracion a NEXTRY.

Pruebas negativas de fuente real en lenguaje/pruebas_usuario_nxf.py: afirmar
falso, mientras verdadero e intento leer_puerto. Las tres conservan consola;
el bucle se registra como limite PIT. El script restaura imagen por defecto
en finally. Binario del ejemplo: 437 bytes; verificado contra el ELF embebido.
