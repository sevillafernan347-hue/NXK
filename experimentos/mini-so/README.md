# MINI NXK: interfaz experimental

Abrir VER-MINI-NXK.cmd en la carpeta principal. Construye la imagen grafica
si falta y abre QEMU. Para reconstruir: VER-MINI-NXK.cmd -Recompilar.

La pantalla y terminal se dibujan desde el kernel. Escribe comandos en
minusculas y pulsa Enter: version, mem, archivos, proceso demo, usuario nextry.
La fuente propia muestra letras en mayusculas; el comando conserva lo escrito.
Retroceso borra; Ctrl+Alt+G libera la captura de QEMU. Los paneles son fijos y
se usa teclado: todavia no hay raton, ventanas movibles ni aplicaciones graficas.

El experimento usa VGA BIOS13h320x200 con fuente5x7 original y framebuffer
supervisor. Referencia del hardware: [manual tecnico IBM VGA/XGA](https://bitsavers.trailing-edge.com/pdf/ibm/pc/cards/IBM_VGA_XGA_Technical_Reference_Manual_May92.pdf).
No importa codigo de otro kernel. Driver puente C, entrada BIOS generada con
NEXTRY ASM. Solo x86; build.ps1 -Interfaz genera build/x86-interfaz separado.
El arranque normal sigue disponible; la interfaz no incluye mas controladores.

Verificacion: `tools/python/python.exe experimentos/mini-so/probar.py` arranca
otra VM sin ventana, captura pixeles, envia teclas PS2 y comprueba version,
memoria, escritura/lectura de archivos, NEXTRY, procesos, scroll y retroceso.
Guarda captura en `build/x86-interfaz/mini-nxk.png`. Prueba superada en64MiB;
los discos originales permanecieron intactos. Tambien se observo la ventana GTK.

QEMU usa una copia de la imagen con snapshot. Los archivos creados en RAM
son temporales. No accede al disco fisico de la laptop ni instala NXK en ella.
