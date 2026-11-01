# Empezar a construir NXK

## Las dos aplicaciones

- **Visual Studio Code**: editar y organizar nuestro codigo. Abre `ABRIR-PROYECTO.cmd`.
- **QEMU**: ejecutar el kernel dentro de una computadora virtual. Abre `INICIAR-NXK.cmd`.

No hace falta iniciar sesion en GitHub para usar el editor. Si aparece su bienvenida, puedes elegir continuar sin iniciar sesion. VS Code puede solicitar que revises la confianza de la carpeta antes de habilitar tareas; revisa los archivos del proyecto y decide en el propio editor.

## Tu primera prueba

1. Abre la computadora virtual y haz clic dentro de su pantalla.
2. Escribe `help` y pulsa Enter: deben aparecer los comandos.
3. Escribe `version`: debe responder NXK 0.1.
4. Escribe `mem`: muestra el uso del asignador de memoria de laboratorio.
5. Para liberar el raton y el teclado pulsa Ctrl+Alt+G. Para terminar, cierra QEMU.

Tambien puedes probar `ejecutar hola.nxe` (aplicacion nativa de NXK) y, en 32 bits, `ejecutar hola.exe` (EXE Win32 de demostracion). No hay soporte general de aplicaciones Windows.

Para escribir en nuestro lenguaje, abre `EDITAR-APP.cmd` y sigue `lenguaje/REGLAS.md`. Los fuentes son `.nxf`; los ejecutables propios son `.nxe`.

QEMU es una PC virtual en una ventana. No simula la carcasa de una laptop ni representa todas las piezas de tu laptop real.

## Tu primer cambio de codigo

1. Abre `kernel/main.c` en VS Code.
2. Busca el mensaje `NXK 0.1 - kernel original` y cambia su texto, conservando la macro que indica la arquitectura.
3. Guarda el archivo y pulsa Ctrl+Shift+B.
4. Cierra la maquina virtual anterior y vuelve a abrir `INICIAR-NXK.cmd`.
5. Comprueba que aparece tu nuevo mensaje. Una ventana que ya estaba abierta sigue ejecutando la imagen anterior cargada en RAM.

## Lo siguiente que construiremos

La prioridad tecnica es una **IDT**, la tabla que indica al procesador que hacer cuando sucede una excepcion. Actualmente un error grave del kernel puede detener o reiniciar la maquina virtual sin un diagnostico propio. Despues construiremos el mapa de RAM, el asignador de paginas y el temporizador.

## Orden de estudio recomendado

1. **C**: variables, funciones, arrays, punteros, estructuras, operaciones de bits y memoria. Empieza con Beej o The C Book.
2. **Hardware**: binario y hexadecimal; registros, RAM, caché, buses, instrucciones y modos del procesador. Empieza con Basic Computer Architecture o Computer Organization and Design Fundamentals.
3. **Ensamblador**: registros, pila, llamadas, banderas y acceso a puertos. Distingue x86 de x86-64 y la sintaxis Intel de AT&T: no se mezclan literalmente.
4. **Sistemas operativos**: memoria virtual, interrupciones, concurrencia, procesos y archivos. Empieza con Think OS; despues OSTEP y The Little Book About OS Development.
5. **Algoritmos**: listas, colas, tablas hash, arboles, complejidad y fundamentos matematicos.
6. **C++**: tipos, RAII y plantillas. En un kernel no aparecen automaticamente su biblioteca estandar, excepciones ni runtime.
7. **Python**: automatizar compilaciones, generar archivos y comprobar resultados.
8. **Java y JavaScript**: aplicaciones y herramientas; necesitan un entorno de ejecucion que NXK aun no tiene.
9. **Redes y seguridad**: protocolos, aislamiento, permisos y diseno seguro.

No necesitas leer los 200 titulos antes de escribir codigo. Alterna un tema pequeno con una prueba en NXK. Algunos manuales son historicos y sirven para entender fundamentos; para implementar una plataforma concreta consulta su documentacion vigente.
