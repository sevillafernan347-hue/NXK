# Herramientas externas

NXK no incorpora el kernel de Linux. Usa herramientas independientes para construir y probar su codigo.

| Herramienta | Uso | Origen |
|---|---|---|
| Zig 0.15.2 (Clang/LLD) | Compilar C/ensamblador y enlazar | https://ziglang.org/download/ |
| QEMU 11.1.0, build de Windows 20260811 | Emular la PC de pruebas | https://qemu.weilnetz.de/w64/ (enlazado desde qemu.org) |
| Visual Studio Code | Editor de escritorio | https://code.visualstudio.com/ |
| Microsoft C/C++ | Asistencia de edicion | Extension `ms-vscode.cpptools` |
| x86 and x86_64 Assembly | Coloreado del ensamblador | Extension `13xforever.language-x86-64-assembly` |
| Python 3.12.10 embebido | Ejecutar el compilador de NXK Espanol en la laptop de desarrollo | https://www.python.org/ftp/python/3.12.10/ |

Python se guarda en `tools/python`, con su licencia original. Es una dependencia de construccion para Windows x64 y no forma parte de las aplicaciones NXE o del kernel ejecutado en la ASUS de 32 bits.

Zig y QEMU se guardan en `tools/`. Sus archivos de licencia se conservan en esas carpetas. VS Code se instala para el usuario de Windows. Las descargas de Zig y QEMU se comprobaron contra los hashes publicados por sus distribuidores; winget verifico el instalador de VS Code.

Los manuales y libros de Biblioteca conservan sus licencias y atribuciones. Su disponibilidad gratuita no implica que se permita redistribuirlos o modificarlos sin condiciones.
