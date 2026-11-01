"""NEXTRY CPL3 leyendo NXFS; restaura imagen normal al terminar."""
import pathlib,subprocess,os,sys,time
sys.path.insert(0,str(pathlib.Path(__file__).resolve().parent))
from crear_volumen import crear
raiz=pathlib.Path(__file__).resolve().parents[1]
entorno={k:v for k,v in os.environ.items() if k.lower()!='psmodulepath'}
construir=['powershell.exe','-NoProfile','-ExecutionPolicy','Bypass','-File',str(raiz/'build.ps1'),'-Architecture','x86']
carpeta=raiz/'build/x86/usuario-volumen-pruebas';carpeta.mkdir(exist_ok=True)
base=crear({'leeme.txt':b'NEXTRY_DISCO_VALIDO\n'})
corrupto=bytearray(base);corrupto[512]^=1
casos={'correcto':base,'corrupto':corrupto,'grande':crear({'leeme.txt':b'G'*257}),
       'ausente':crear({'otro.txt':b'otro'}),'sin_montar':base}
try:
    subprocess.run(construir+['-FuenteUsuario','lenguaje/ejemplos/usuario_volumen.nxf'],cwd=raiz,env=entorno,check=True,capture_output=True)
    for nombre,datos in casos.items():
        disco=carpeta/f'{nombre}.img';disco.write_bytes(datos);log=carpeta/f'{nombre}.log'
        with log.open('wb',buffering=0) as salida:
            p=subprocess.Popen([str(raiz/'tools/qemu/qemu-system-i386.exe'),'-machine','pc','-accel','tcg',
                '-cpu','qemu32','-m','32M','-drive','file=build/x86/nxk.img,format=raw,if=ide,index=0',
                '-drive',f'file={disco.as_posix()},format=raw,if=ide,index=1,snapshot=on','-snapshot',
                '-display','none','-serial','stdio','-monitor','none','-no-reboot'],cwd=raiz,
                stdin=subprocess.PIPE,stdout=salida,stderr=salida)
            def esperar(texto,n=1):
                fin=time.monotonic()+15
                while time.monotonic()<fin:
                    if log.read_bytes().count(texto)>=n:return
                    if p.poll() is not None:raise RuntimeError('QEMU termino')
                    time.sleep(.03)
                raise RuntimeError(log.read_text(errors='replace'))
            def enviar(texto):
                for v in texto:p.stdin.write(bytes([v]));p.stdin.flush();time.sleep(.003)
            try:
                esperar(b'nxk> ')
                if nombre!='sin_montar':enviar(b'volumen montar\n');esperar(b'Volumen NXFS montado')
                enviar(b'usuario nextry\n')
                if nombre=='correcto':
                    esperar(b'NEXTRY_DISCO_VALIDO');esperar(b'Aplicacion NEXTRY protegida termino.')
                    enviar(b'escribir leeme.txt SOMBRA_RAM\nusuario nextry\n')
                    esperar(b'SOMBRA_RAM',2);esperar(b'Aplicacion NEXTRY protegida termino.',2)
                    enviar(b'escribir leeme.txt \nusuario nextry\n')
                    esperar(b'Aplicacion NEXTRY protegida termino.',3)
                    if log.read_bytes().count(b'NEXTRY_DISCO_VALIDO')!=1:raise RuntimeError('RAM vacio no debe buscar disco')
                    enviar(b'borrar leeme.txt\nusuario nextry\n')
                    esperar(b'NEXTRY_DISCO_VALIDO',2);esperar(b'Aplicacion NEXTRY protegida termino.',4)
                    enviar(b'leer leeme.txt\n');esperar(b'NEXTRY_DISCO_VALIDO',3)
                else:
                    esperar(b'Aplicacion NEXTRY fallo.')
                    if b'NEXTRY_DISCO_VALIDO' in log.read_bytes():raise RuntimeError('Contenido invalido publicado')
                enviar(b'help\n');esperar(b'help | version')
                print('USUARIO NXFS '+nombre+': correcto.')
            finally:p.terminate();p.wait(timeout=5)
        if disco.read_bytes()!=datos:raise RuntimeError('La imagen base cambio')
finally:subprocess.run(construir,cwd=raiz,env=entorno,check=True,capture_output=True)
