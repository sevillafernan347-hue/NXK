"""Prueba NXFS1 en discos virtuales IDE secundarios de solo lectura."""
import pathlib,re,subprocess,sys,time,struct
sys.path.insert(0,str(pathlib.Path(__file__).resolve().parent))
from crear_volumen import crear,huella
raiz=pathlib.Path(__file__).resolve().parents[1]
carpeta=raiz/'build/x86/volumen-pruebas'; carpeta.mkdir(exist_ok=True)
base=crear({'leeme.txt':b'NXFS: lectura persistente de prueba.\n','largo.txt':b'Z'*1100,'vacio.txt':b'','exacto.txt':b'X'*256})
casos={}
casos['correcto']=(base,True)
casos['metadatos']=(base,True)
casos['ciclo']=(base,True)
casos['reinicio-canal']=(base,True)
casos['vacio']=(crear({}),True)
nombre_maximo = 'a' * 23
lleno = {f'dato{i}.txt': bytes([65+i]) for i in range(7)}
lleno[nombre_maximo] = b'MAXIMO_NXFS'
casos['ocho-entradas']=(crear(lleno),True)
# Patron comprobado completo, incluido el ultimo byte del octavo sector.
patron = bytes(65 + (i+i//251)%26 for i in range(4096))
maximo = crear({'maximo.txt':patron})
casos['lectura-maxima']=(maximo,True)
casos['tarea-por-turnos']=(crear({'maximo.txt':patron,'largo.txt':b'Z'*1100,'vacio.txt':b''}),True)
corrupto_final = bytearray(maximo)
corrupto_final[512+len(patron)-1] ^= 1
casos['contenido-final']=(corrupto_final,True)
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
        def comando(texto):
            desde=log.stat().st_size
            prompts=log.read_bytes().count(b'nxk> ')
            enviar(texto+b'\n')
            esperar(b'nxk> ',prompts+1)
            return log.read_bytes()[desde:]
        def heap_usado():
            return int(re.search(rb'Heap fijo usado: (\d+)',comando(b'mem'))[1])
        try:
            esperar(b'nxk> ')
            desmontado=b'Volumen NXFS desmontado.'
            if nombre=='ciclo':
                assert desmontado in comando(b'volumen estado')
                assert desmontado in comando(b'volumen verificar')
            montaje=b'Volumen NXFS montado en solo lectura.'
            assert (montaje if valido else b'Volumen no disponible o invalido.') in comando(b'volumen montar')
            if nombre=='correcto':
                antes=heap_usado()
                for _ in range(2):
                    respuesta=comando(b'volumen verificar')
                    for archivo in (b'leeme.txt',b'largo.txt',b'vacio.txt',b'exacto.txt'):
                        assert archivo+b': OK' in respuesta
                    assert b'NXFS: correctos 4 de 4.' in respuesta
                    assert b'lectura persistente de prueba' not in respuesta
                    assert heap_usado()==antes+4096
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

            elif nombre=='metadatos':
                estado=comando(b'discos estado')
                assert b'comando 0x00000020 emitido 1 LBA 0' in estado, estado
                heap=heap_usado()
                assert b'Archivo NXFS: 1100 bytes' in comando(b'archivo info largo.txt')
                assert b'Archivo NXFS: 0 bytes' in comando(b'archivo info vacio.txt')
                assert b'Archivo no encontrado.' in comando(b'archivo info ausente')
                assert b'Nombre de archivo invalido.' in comando(b'archivo info ../x')
                assert b'No se pudo copiar.' in comando(b'copiar leeme.txt exacto.txt')
                assert comando(b'discos estado')==estado
                assert heap_usado()==heap, 'Consultar y rechazar copia no deben reservar memoria'
                assert b'Archivo guardado en RAM.' in comando(b'escribir largo.txt sombra')
                assert b'Archivo RAM: 6 bytes' in comando(b'archivo info largo.txt')
                assert b'Archivo borrado.' in comando(b'borrar largo.txt')
                assert b'Archivo NXFS: 1100 bytes' in comando(b'archivo info largo.txt')
                assert comando(b'discos estado')==estado

            elif nombre=='ciclo':
                assert montaje+b' Archivos: 4' in comando(b'volumen estado')
                assert b'largo.txt' in comando(b'volumen listar')
                assert b'Z'*1100 in comando(b'volumen leer largo.txt')
                assert b'Archivo guardado en RAM.' in comando(b'escribir fuente local')
                heap=heap_usado()
                ata=comando(b'discos estado')
                for _ in range(2):
                    assert desmontado in comando(b'volumen desmontar')
                    assert desmontado in comando(b'volumen estado')
                assert b'largo.txt' not in comando(b'volumen listar')
                assert b'Archivo no encontrado.' in comando(b'archivo info largo.txt')
                assert b'No se pudo leer.' in comando(b'leer largo.txt')
                assert b'No se pudo leer o verificar' in comando(b'volumen leer largo.txt')
                assert ata==comando(b'discos estado')
                assert heap_usado()==heap
                assert b'Archivo copiado a RAM.' in comando(b'copiar fuente largo.txt')
                assert montaje in comando(b'volumen montar')
                assert montaje+b' Archivos: 4' in comando(b'volumen estado')
                assert b'Archivo RAM: 5 bytes' in comando(b'archivo info largo.txt')
                assert b'Z'*1100 in comando(b'volumen leer largo.txt')
                assert desmontado in comando(b'volumen desmontar')
                assert b'Archivo RAM: 5 bytes' in comando(b'archivo info largo.txt')
                assert b'local' in comando(b'leer largo.txt')
                assert heap_usado()==heap

            elif nombre=='reinicio-canal':
                assert b'Archivo guardado en RAM.' in comando(b'escribir local RAM_INTACTA')
                assert b'NXFS: lectura persistente' in comando(b'volumen leer leeme.txt')
                memoria=heap_usado()
                for _ in range(2):
                    estado=comando(b'discos estado')
                    assert b'Canal IDE reiniciado.' in comando(b'discos reiniciar')
                    assert estado==comando(b'discos estado')
                    assert b'Archivo no encontrado.' in comando(b'archivo info leeme.txt')
                    assert desmontado in comando(b'volumen estado')
                    assert desmontado in comando(b'volumen verificar')
                    assert b'RAM_INTACTA' in comando(b'leer local')
                    assert montaje in comando(b'volumen montar')
                    assert b'NXFS: lectura persistente' in comando(b'volumen leer leeme.txt')
                    assert heap_usado()==memoria

            elif nombre=='vacio':
                assert montaje+b' Archivos: 0' in comando(b'volumen estado')
                antes=heap_usado()
                ata=comando(b'discos estado')
                assert b'NXFS: correctos 0 de 0.' in comando(b'volumen verificar')
                assert heap_usado()==antes and comando(b'discos estado')==ata
                assert desmontado in comando(b'volumen desmontar')
                assert desmontado in comando(b'volumen estado')
                assert montaje in comando(b'volumen montar')
                assert montaje+b' Archivos: 0' in comando(b'volumen estado')

            elif nombre=='contenido':
                estado=comando(b'discos estado')
                assert b'Archivo NXFS:' in comando(b'archivo info leeme.txt')
                assert b'No se pudo copiar.' in comando(b'copiar exacto.txt leeme.txt')
                assert comando(b'discos estado')==estado
                assert b'Archivo NXFS:' in comando(b'archivo info leeme.txt')
                respuesta=comando(b'volumen verificar')
                assert b'leeme.txt: huella incorrecta' in respuesta
                assert b'exacto.txt: OK' in respuesta
                assert b'NXFS: correctos 3 de 4.' in respuesta
                assert b'lectura persistente de prueba' not in respuesta
                enviar(b'volumen leer leeme.txt\n'); esperar(b'No se pudo leer o verificar')
                enviar(b'copiar leeme.txt rechazado\n'); esperar(b'No se pudo copiar.',2)
                enviar(b'leer rechazado\n'); esperar(b'No se pudo leer.')

            elif nombre=='ocho-entradas':
                enviar(b'volumen listar\n'); esperar(nombre_maximo.encode())
                enviar(f'volumen leer {nombre_maximo}\n'.encode()); esperar(b'MAXIMO_NXFS')

            elif nombre=='lectura-maxima':
                antes=heap_usado()
                assert b'Archivo NXFS: 4096 bytes' in comando(b'archivo info maximo.txt')
                assert b'No se pudo leer o verificar' in comando(b'volumen leer ausente')
                assert heap_usado()==antes, 'Una consulta fallida no debe reservar el buffer'
                for _ in range(2):
                    assert patron in comando(b'volumen leer maximo.txt')
                    assert heap_usado()-antes==4096, 'NXFS debe usar un solo buffer de 4 KiB'

            elif nombre=='tarea-por-turnos':
                estado=comando(b'discos estado')
                antes=heap_usado()
                assert b'NXFS tarea pendiente 0/4096 bytes.' in comando(b'volumen iniciar maximo.txt')
                assert b'NXFS ya tiene una tarea pendiente.' in comando(b'volumen iniciar largo.txt')
                assert b'NXFS tarea pendiente 0/4096 bytes.' in comando(b'volumen progreso')
                assert estado==comando(b'discos estado') and heap_usado()==antes
                assert b'NXFS tarea pendiente 512/4096 bytes.' in comando(b'volumen paso')
                assert heap_usado()==antes
                # La demo despacha dos procesos en tres rondas. Cada turno
                # real agrega un sector, sin completar toda la tarea de golpe.
                assert b'registros y punto de ejecucion conservados' in comando(b'proceso demo')
                assert b'NXFS tarea pendiente 3584/4096 bytes.' in comando(b'volumen progreso')
                assert b'NXFS tarea verificada 4096/4096 bytes.' in comando(b'volumen paso')
                assert b'NXFS tarea verificada 4096/4096 bytes.' in comando(b'volumen paso')

                assert b'NXFS tarea pendiente 0/1100 bytes.' in comando(b'volumen iniciar largo.txt')
                assert b'NXFS tarea pendiente 512/1100 bytes.' in comando(b'volumen paso')
                assert b'NXFS tarea cancelada 512/1100 bytes.' in comando(b'volumen cancelar')
                assert b'NXFS tarea cancelada 512/1100 bytes.' in comando(b'volumen paso')
                assert b'NXFS tarea pendiente 0/1100 bytes.' in comando(b'volumen iniciar largo.txt')
                assert b'NXFS tarea pendiente 512/1100 bytes.' in comando(b'volumen paso')
                assert b'Canal IDE reiniciado.' in comando(b'discos reiniciar')
                assert b'NXFS tarea invalidada 512/1100 bytes.' in comando(b'volumen paso')
                assert montaje in comando(b'volumen montar')
                estado=comando(b'discos estado')
                assert b'NXFS tarea verificada 0/0 bytes.' in comando(b'volumen iniciar vacio.txt')
                assert estado==comando(b'discos estado')

            elif nombre=='contenido-final':
                respuesta=comando(b'volumen verificar')
                assert b'maximo.txt: huella incorrecta' in respuesta
                assert b'NXFS: correctos 0 de 1.' in respuesta
                assert patron[:512] not in respuesta
                respuesta=comando(b'volumen leer maximo.txt')
                assert b'No se pudo leer o verificar' in respuesta
                assert patron[:512] not in respuesta, 'NXFS publico bytes antes de verificar'
                assert b'NXFS tarea pendiente 0/4096 bytes.' in comando(b'volumen iniciar maximo.txt')
                for _ in range(7):
                    assert b'NXFS tarea pendiente' in comando(b'volumen paso')
                assert b'NXFS tarea huella incorrecta 4096/4096 bytes.' in comando(b'volumen paso')

            elif not valido:
                assert desmontado in comando(b'volumen estado')
                enviar(b'volumen leer leeme.txt\n'); esperar(b'No se pudo leer o verificar')

            enviar(b'help\n'); esperar(b'help | version')
            print('NXFS '+nombre+': correcto.')
        finally: p.terminate(); p.wait(timeout=5)
    if disco.read_bytes()!=datos: raise RuntimeError('La imagen base cambio')
