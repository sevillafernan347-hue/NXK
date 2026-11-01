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
    try:
        esperar(b'nxk> ')
        for i in range(2):
            proceso.stdin.write(b'usuario fallos\n'); proceso.stdin.flush()
            esperar(b'Cuatro fallos de usuario contenidos;',i+1)
            proceso.stdin.write(b'usuario\n'); proceso.stdin.flush()
            esperar(b'control devuelto al kernel.',i+1)
        proceso.stdin.write(b'help\n'); proceso.stdin.flush()
        esperar(b'help | version')
        print('CONSOLA OK: fallos recuperados, programa repetido y help operativo.')
    finally:
        proceso.terminate(); proceso.wait(timeout=5)
