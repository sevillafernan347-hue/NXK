"""Comprueba fallos de fuentes NEXTRY reales en CPL3; restaura imagen por defecto."""
import os,pathlib,subprocess,time
raiz=pathlib.Path(__file__).resolve().parents[1]
entorno={k:v for k,v in os.environ.items() if k.lower()!='psmodulepath'}
construir=['powershell.exe','-NoProfile','-ExecutionPolicy','Bypass','-File',str(raiz/'build.ps1'),'-Architecture','x86']
casos={
 'afirmacion':'funcion inicio\nafirmar falso\nfin',
 'bucle':'funcion inicio\nmientras verdadero\nfin\nfin',
 'puerto':'funcion inicio\nentero dato = 0\nleer_puerto dato, 0x3fd\nfin',
 'archivo_ausente':'funcion inicio\nmostrar_archivo "no_existe"\nfin',
 'nombre_invalido':'funcion inicio\nguardar_archivo "../x", "dato"\nfin'}
try:
    for nombre,fuente in casos.items():
        archivo=raiz/f'build/x86/negativo-{nombre}.nxf'; archivo.write_text(fuente)
        subprocess.run(construir+['-FuenteUsuario',str(archivo)],cwd=raiz,env=entorno,check=True,capture_output=True)
        log=raiz/f'build/x86/usuario-nxf-{nombre}.log'
        with log.open('wb',buffering=0) as salida:
            p=subprocess.Popen([str(raiz/'tools/qemu/qemu-system-i386.exe'),'-machine','pc','-accel','tcg',
                '-cpu','qemu32','-m','32M','-drive','file=build/x86/nxk.img,format=raw,if=ide',
                '-snapshot','-display','none','-serial','stdio','-monitor','none','-no-reboot'],
                cwd=raiz,stdin=subprocess.PIPE,stdout=salida,stderr=salida)
            def esperar(texto):
                plazo=time.monotonic()+15
                while time.monotonic()<plazo:
                    if texto in log.read_bytes(): return
                    if p.poll() is not None: raise RuntimeError('QEMU termino inesperadamente')
                    time.sleep(.05)
                raise RuntimeError(log.read_bytes().decode(errors='replace'))
            def enviar(texto):
                for byte in texto:
                    p.stdin.write(bytes([byte])); p.stdin.flush(); time.sleep(.003)
            try:
                esperar(b'nxk> '); enviar(b'usuario nextry\n')
                esperar(b'Aplicacion NEXTRY fallo.')
                enviar(b'usuario estado\nhelp\n'); esperar(b'help | version')
                datos=log.read_bytes()
                if nombre=='bucle':
                    if b'fallo, vector 32' not in datos: raise RuntimeError('No se aplico limite PIT')
                elif b'Error de aplicacion NEXTRY' not in datos: raise RuntimeError('Falta error de runtime')
                print(f'NEXTRY {nombre}: fallo contenido y consola operativa.')
            finally: p.terminate(); p.wait(timeout=5)
finally:
    subprocess.run(construir,cwd=raiz,env=entorno,check=True,capture_output=True)
