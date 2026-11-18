"""Prueba NXFS1 en discos virtuales IDE secundarios de solo lectura."""
import pathlib,subprocess,sys,time,struct
sys.path.insert(0,str(pathlib.Path(__file__).resolve().parent))
from crear_volumen import crear,huella
raiz=pathlib.Path(__file__).resolve().parents[1]
carpeta=raiz/'build/x86/volumen-pruebas'; carpeta.mkdir(exist_ok=True)
base=crear({'leeme.txt':b'NXFS: lectura persistente de prueba.\n','largo.txt':b'Z'*1100,'vacio.txt':b'','exacto.txt':b'X'*256})
casos={}
casos['correcto']=(base,True)
casos['vacio']=(crear({}),True)
nombre_maximo = 'a' * 23
lleno = {f'dato{i}.txt': bytes([65+i]) for i in range(7)}
lleno[nombre_maximo] = b'MAXIMO_NXFS'
casos['ocho-entradas']=(crear(lleno),True)
for nombre in ('cabecera','cantidad','nombre','limite','solapamiento','tamano'):
    b=bytearray(base)
    if nombre=='cabecera': b[0]^=1
    if nombre=='cantidad': struct.pack_into('<I',b,8,9)
    if nombre=='nombre': b[16:40]=b'a'*24
    if nombre=='limite': struct.pack_into('<I',b,44,2048)
    if nombre=='solapamiento': struct.pack_into('<I',b,84,1)
    if nombre=='tamano': struct.pack_into('<I',b,40,4097)
    struct.pack_into('<I',b,508,huella(b[:508])); casos[nombre]=(b,False)
b=bytearray(base); b[508]^=1; casos['huella']=(b,False)
b=bytearray(base); b[512]^=1; casos['contenido']=(b,True)
# La huella recalculada aísla esta validación de la detección de corrupción.
for nombre,pos in (
    ('reservado-cabecera',15), ('reservado-entrada-final',16+3*40+39),
    ('relleno-nombre-final',16+3*40+23), ('entrada-inactiva',16+4*40),
    ('relleno-sector',507),
):
    b=bytearray(base); b[pos]=1
    struct.pack_into('<I',b,508,huella(b[:508])); casos[nombre]=(b,False)
for nombre,(datos,valido) in casos.items():
    disco=carpeta/f'{nombre}.img'; disco.write_bytes(datos)
    log=carpeta/f'{nombre}.log'
    with log.open('wb',buffering=0) as salida:
        p=subprocess.Popen([str(raiz/'tools/qemu/qemu-system-i386.exe'),'-machine','pc','-accel','tcg',
            '-cpu','qemu32','-m','32M','-drive','file=build/x86/nxk.img,format=raw,if=ide,index=0',
            '-drive',f'file={disco.as_posix()},format=raw,if=ide,index=1,snapshot=on','-snapshot',
            '-display','none','-serial','stdio','-monitor','none','-no-reboot'],cwd=raiz,
            stdin=subprocess.PIPE,stdout=salida,stderr=salida)
        def esperar(texto,cantidad=1):
            fin=time.monotonic()+15
            while time.monotonic()<fin:
                if log.read_bytes().count(texto)>=cantidad: return
                if p.poll() is not None: raise RuntimeError('QEMU termino: '+log.read_text(errors='replace'))
                time.sleep(.03)
            raise RuntimeError(log.read_text(errors='replace'))
        def enviar(texto):
            for v in texto:
                p.stdin.write(bytes([v])); p.stdin.flush(); time.sleep(.003)
        try:
            esperar(b'nxk> '); enviar(b'volumen montar\n')
            esperar(b'Volumen NXFS montado' if valido else b'Volumen no disponible o invalido.')
            if nombre=='correcto':
                enviar(b'volumen listar\n'); esperar(b'largo.txt')
                enviar(b'volumen leer leeme.txt\n'); esperar(b'NXFS: lectura persistente de prueba.')
                enviar(b'volumen leer largo.txt\n'); esperar(b'Z'*1100)
                enviar(b'volumen leer ausente\n'); esperar(b'No se pudo leer o verificar')
                enviar(b'copiar leeme.txt editable\n'); esperar(b'Archivo copiado a RAM.')
                enviar(b'agregar editable CAMBIO_LOCAL\nleer editable\n'); esperar(b'CAMBIO_LOCAL',2)
                enviar(b'copiar leeme.txt editable\n'); esperar(b'No se pudo copiar.')
                enviar(b'copiar leeme.txt largo.txt\n'); esperar(b'No se pudo copiar.',2)
                enviar(b'copiar largo.txt truncado\n'); esperar(b'No se pudo copiar.',3)
                enviar(b'leer truncado\n'); esperar(b'No se pudo leer.')
                enviar(b'copiar vacio.txt vacio_ram\n'); esperar(b'Archivo copiado a RAM.',2)
                enviar(b'copiar exacto.txt exacto_ram\nleer exacto_ram\n'); esperar(b'X'*256)
                enviar(b'copiar ausente fantasma\n'); esperar(b'No se pudo copiar.',4)
                enviar(b'leer fantasma\n'); esperar(b'No se pudo leer.',2)
                # Borrar la copia no debe borrar el original de disco.
                enviar(b'borrar editable\nvolumen leer leeme.txt\n')
                esperar(b'NXFS: lectura persistente de prueba.',2)

            elif nombre=='contenido':
                enviar(b'volumen leer leeme.txt\n'); esperar(b'No se pudo leer o verificar')
                enviar(b'copiar leeme.txt rechazado\n'); esperar(b'No se pudo copiar.')
                enviar(b'leer rechazado\n'); esperar(b'No se pudo leer.')

            elif nombre=='ocho-entradas':
                enviar(b'volumen listar\n'); esperar(nombre_maximo.encode())
                enviar(f'volumen leer {nombre_maximo}\n'.encode()); esperar(b'MAXIMO_NXFS')

            elif not valido:
                enviar(b'volumen leer leeme.txt\n'); esperar(b'No se pudo leer o verificar')

            enviar(b'help\n'); esperar(b'help | version')
            print('NXFS '+nombre+': correcto.')
        finally: p.terminate(); p.wait(timeout=5)
    if disco.read_bytes()!=datos: raise RuntimeError('La imagen base cambio')
