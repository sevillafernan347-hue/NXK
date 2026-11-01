"""Prueba la consola serie sin interactuar con la ventana QEMU del usuario.
Requiere imagen normal x86 ya construida. No modifica el disco (snapshot).
"""
import pathlib, subprocess, time
raiz=pathlib.Path(__file__).resolve().parents[1]
log=raiz/'build/x86/consola-usuario.log'
with log.open('wb',buffering=0) as salida:
    proceso=subprocess.Popen([str(raiz/'tools/qemu/qemu-system-i386.exe'),
        '-machine','pc','-accel','tcg','-cpu','qemu32','-m','32M',
        '-drive','file=build/x86/nxk.img,format=raw,if=ide','-snapshot',
        '-display','none','-serial','stdio','-monitor','none','-no-reboot'],
        cwd=raiz,stdin=subprocess.PIPE,stdout=salida,stderr=salida)
    def esperar(texto,cantidad=1):
        plazo=time.monotonic()+15
        while time.monotonic()<plazo:
            if log.read_bytes().count(texto)>=cantidad: return
            if proceso.poll() is not None: raise RuntimeError('QEMU termino antes de responder')
            time.sleep(.05)
        raise RuntimeError(log.read_bytes().decode(errors='replace'))
    def enviar(datos):
        # Ritmo de terminal: evita desbordar la FIFO UART con comandos largos.
        for byte in datos:
            proceso.stdin.write(bytes([byte])); proceso.stdin.flush(); time.sleep(.003)
    try:
        esperar(b'nxk> ')
        enviar(b'usuario estado\n')
        esperar(b'Sin ejecuciones de usuario.')
        enviar(b'usuario nulo\nusuario estado\n')
        esperar(b'fallo, vector 14 error 4')
        enviar(b'usuario\nusuario estado\n')
        esperar(b'normal, salida 42')
        for i in range(2):
            enviar(b'usuario fallos\n')
            esperar(b'Nueve casos de usuario contenidos;',i+1)
            enviar(b'usuario\n')
            esperar(b'control devuelto al kernel.',i+2)
        enviar(b'usuario bucle\n')
        esperar(b'Bucle interrumpido por PIT; consola recuperada.')
        enviar(b'usuario historial\n')
        esperar(b'Ejecucion ',10)
        enviar(b'usuario texto\n')
        esperar(b'Hola desde usuario')
        esperar(b'Servicio y rechazos de punteros verificados.')
        enviar(b'usuario nextry\n')
        esperar(b'NEXTRY protegido: aplicacion en modo usuario')
        esperar(b'Archivo creado desde NEXTRY protegido')
        esperar(b'Aplicacion NEXTRY protegida termino.')
        enviar(b'help\n')
        esperar(b'help | version')
        print('CONSOLA OK: fallos recuperados, programa repetido y help operativo.')
    finally:
        proceso.terminate(); proceso.wait(timeout=5)
