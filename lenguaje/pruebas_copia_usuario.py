"""Copias reales NEXTRY CPL3 desde NXFS; nunca escribe el disco base."""
import os,pathlib,subprocess,time,sys
sys.path.insert(0,str(pathlib.Path(__file__).resolve().parent))
from crear_volumen import crear
raiz=pathlib.Path(__file__).resolve().parents[1]
carpeta=raiz/'build/x86/copia-usuario-pruebas';carpeta.mkdir(exist_ok=True)
entorno={k:v for k,v in os.environ.items() if k.lower()!='psmodulepath'}
construir=['powershell.exe','-NoProfile','-ExecutionPolicy','Bypass','-File',str(raiz/'build.ps1'),'-Architecture','x86']
fuente=raiz/'lenguaje/ejemplos/copiar_volumen.nxf'
base=crear({'leeme.txt':b'ORIGEN_COPIADO'})
corrupto=bytearray(base);corrupto[512]^=1
# Cada escenario arranca con un sistema RAM nuevo.
casos=[('correcto',base,b'ORIGEN_COPIADO',b''),
       ('vacio',crear({'leeme.txt':b''}),b'',b''),
       ('exacto',crear({'leeme.txt':b'Q'*256}),b'Q'*256,b''),
       ('grande',crear({'leeme.txt':b'Q'*257}),None,b''),
       ('corrupto',corrupto,None,b''),
       ('ausente',crear({'otro':b'OTRO'}),None,b''),
       ('destino_disco',crear({'leeme.txt':b'ORIGEN','editable':b'NO_TOCAR'}),None,b''),
       ('destino_ram',base,None,b'escribir editable NO_TOCAR\n'),
       ('tabla_llena',base,None,b''.join(f'escribir f{i} X\n'.encode() for i in range(8))),
       ('sin_montar',base,None,b'')]
try:
    subprocess.run(construir+['-FuenteUsuario',str(fuente)],cwd=raiz,env=entorno,check=True,capture_output=True)
    for nombre,datos,esperado,previo in casos:
        disco=carpeta/f'{nombre}.img';disco.write_bytes(datos)
        log=carpeta/f'{nombre}.log'
        with log.open('wb',buffering=0) as salida:
            p=subprocess.Popen([str(raiz/'tools/qemu/qemu-system-i386.exe'),'-machine','pc','-accel','tcg',
                '-cpu','qemu32','-m','32M','-drive','file=build/x86/nxk.img,format=raw,if=ide,index=0',
                '-drive',f'file={disco.as_posix()},format=raw,if=ide,index=1,snapshot=on',
                '-snapshot','-display','none','-serial','stdio','-monitor','none','-no-reboot'],
                cwd=raiz,stdin=subprocess.PIPE,stdout=salida,stderr=salida)
            def esperar(texto,cantidad=1):
                fin=time.monotonic()+15
                while time.monotonic()<fin:
                    if log.read_bytes().count(texto)>=cantidad:return
                    if p.poll() is not None:break
                    time.sleep(.03)
                raise RuntimeError(log.read_text(errors='replace'))
            def enviar(texto):
                for v in texto:p.stdin.write(bytes([v]));p.stdin.flush();time.sleep(.003)
            try:
                esperar(b'nxk> ')
                if nombre!='sin_montar':
                    enviar(b'volumen montar\n');esperar(b'Volumen NXFS montado')
                if previo:
                    enviar(previo)
                    esperar(b'Archivo guardado en RAM.',8 if nombre=='tabla_llena' else 1)
                enviar(b'usuario nextry\n')
                if esperado is not None:
                    esperar(b'Aplicacion NEXTRY protegida termino.')
                    if esperado:esperar(esperado)
                    enviar(b'agregar editable _LOCAL\n')
                    if nombre=='exacto':esperar(b'No se pudo escribir.')
                    else:
                        esperar(b'Archivo guardado en RAM.')
                        enviar(b'leer editable\n');esperar(esperado+b'_LOCAL')
                else:
                    esperar(b'Aplicacion NEXTRY fallo.')
                    if b'COPIA_CPL3_COMPLETA' in log.read_bytes():raise RuntimeError('Error no propagado')
                    enviar(b'leer editable\n')
                    esperar(b'NO_TOCAR' if nombre in ('destino_ram','destino_disco') else b'No se pudo leer.')
                enviar(b'help\n');esperar(b'help | version')
                print('COPIA CPL3 '+nombre+': correcto.')
            finally:p.terminate();p.wait(timeout=5)
        if disco.read_bytes()!=datos:raise RuntimeError('La imagen base cambio')
finally:
    subprocess.run(construir,cwd=raiz,env=entorno,check=True,capture_output=True)
