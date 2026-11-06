"""Carga NXE2 desde NXFS en QEMU; rechaza cabeceras y contiene fallos de app."""
import pathlib,subprocess,sys,os,time,struct
sys.path.insert(0,str(pathlib.Path(__file__).resolve().parent))
from crear_volumen import crear
from empaquetar_usuario import empaquetar
raiz=pathlib.Path(__file__).resolve().parents[1];carpeta=raiz/'build/x86/nxe-disco-pruebas';carpeta.mkdir(exist_ok=True)
entorno={k:v for k,v in os.environ.items() if k.lower()!='psmodulepath'}
ps=['powershell.exe','-NoProfile','-ExecutionPolicy','Bypass','-File']
try:
    subprocess.run(ps+[str(raiz/'compilar-usuario.ps1'),'-Fuente','lenguaje/ejemplos/usuario_disco.nxf'],cwd=raiz,env=entorno,check=True,capture_output=True)
    base=(raiz/'build/x86/usuario-nxf/programa.nxe').read_bytes()
    casos={'correcto':(base,'correcto')}
    cambios={'firma':(0,0),'version':(4,1),'arquitectura':(6,2),'cabecera':(8,32),'base':(12,0),
             'entrada':(16,1),'longitud':(20,0),'memoria':(24,0),'abi':(32,99),'reservado':(36,1)}
    for nombre,(pos,v) in cambios.items():
        b=bytearray(base);struct.pack_into('<H' if pos in (4,6) else '<I',b,pos,v);casos[nombre]=(b,'rechazo')
    b=bytearray(base);b[-1]^=1;casos['checksum']=(b,'rechazo')
    casos['truncado']=(base[:63],'rechazo')
    elf=bytearray(52);elf[:7]=b'\x7fELF\x01\x01\x01';struct.pack_into('<HHII',elf,16,2,3,1,0x400000)
    casos['bucle']=(empaquetar(elf,b'\xeb\xfe'),'bucle')
    casos['registros']=(empaquetar(elf,bytes.fromhex('09 d8 09 c8 09 d0 09 f0 09 f8 09 e8 85 c0 74 02 0f 0b 31 c0 cd 80')),'registros')
    codigo=bytearray(136);codigo[:10]=bytes.fromhex('31 c0 62 05 80 00 40 00 0f 0b');struct.pack_into('<II',codigo,128,1,2)
    casos['limites']=(empaquetar(elf,codigo),'limites')
    # Un selector nulo en CPL3 es legal: ninguna entrada al kernel debe usarlo.
    for nombre,segmentos in [('ds', '8e d8'), ('es', '8e c0'), ('todos', '8e d8 8e c0 8e e0 8e e8')]:
        inicio=bytes.fromhex('31 c0 '+segmentos)
        casos['segmentos_'+nombre+'_salida']=(empaquetar(elf,inicio+bytes.fromhex('cd 80')),'registros')
        casos['segmentos_'+nombre+'_fallo']=(empaquetar(elf,inicio+bytes.fromhex('0f 0b')),'segmentos')

    for nombre,(app,tipo) in casos.items():
        disco=carpeta/f'{nombre}.img'
        archivos={'disco.nxe':app,'leeme.txt':b'Datos leidos por aplicacion NXE2 desde NXFS\n'}
        if nombre=='correcto':
            malo=bytearray(base);malo[4]=1;archivos['malo.nxe']=malo
        datos=crear(archivos);disco.write_bytes(datos);log=carpeta/f'{nombre}.log' 
        with log.open('wb',buffering=0) as salida:
            p=subprocess.Popen([str(raiz/'tools/qemu/qemu-system-i386.exe'),'-machine','pc','-accel','tcg','-cpu','qemu32','-m','32M',
                '-drive','file=build/x86/nxk.img,format=raw,if=ide,index=0','-drive',f'file={disco.as_posix()},format=raw,if=ide,index=1,snapshot=on',
                '-snapshot','-display','none','-serial','stdio','-monitor','none','-no-reboot'],cwd=raiz,stdin=subprocess.PIPE,stdout=salida,stderr=salida)
            def esperar(texto,cantidad=1):
                fin=time.monotonic()+15
                while time.monotonic()<fin:
                    if log.read_bytes().count(texto)>=cantidad:return
                    if p.poll() is not None:raise RuntimeError('QEMU termino')
                    time.sleep(.03)
                raise RuntimeError(log.read_text(errors='replace'))
            def enviar(texto):
                for v in texto:p.stdin.write(bytes([v]));p.stdin.flush();time.sleep(.003)
            try:
                esperar(b'nxk> ')
                if tipo=='correcto':
                    enviar(b'tareas ocho\n');esperar(b'Ocho tareas: argumentos, pilas, esperas y finalizacion verificadas.')
                enviar(b'volumen montar\n');esperar(b'Volumen NXFS montado')
                enviar(b'ejecutar usuario disco.nxe\n')
                if tipo in ('correcto','registros'):
                    if tipo=='correcto':
                        esperar(b'Aplicacion NEXTRY cargada desde disco')
                        esperar(b'Datos leidos por aplicacion NXE2 desde NXFS')
                        esperar(b'COPIA_NXE2_EDITADA')
                    esperar(b'Aplicacion NXE2 termino; consola recuperada.')
                else:esperar(b'No se pudo cargar o ejecutar NXE2.')
                enviar(b'usuario estado\n')
                if tipo=='rechazo':esperar(b'Sin ejecuciones de usuario.')
                elif tipo=='bucle':esperar(b'fallo, vector 32')
                elif tipo=='limites':esperar(b'fallo, vector 5')
                elif tipo=='segmentos':esperar(b'fallo, vector 6')
                else:
                    esperar(b'normal, salida 0')
                    enviar(b'mem\n');esperar(b'Heap disponible:')
                    enviar(b'ejecutar usuario malo.nxe\n');esperar(b'No se pudo cargar o ejecutar NXE2.')
                    enviar(b'ejecutar usuario disco.nxe\n');esperar(b'Aplicacion NXE2 termino; consola recuperada.',2)
                    enviar(b'ejecutar usuario disco.nxe\n');esperar(b'Aplicacion NXE2 termino; consola recuperada.',3)
                    if tipo=='correcto':esperar(b'COPIA_NXE2_EDITADA',3)
                    enviar(b'mem\n');esperar(b'Heap disponible:',2)
                    import re
                    usos=re.findall(rb'Heap fijo usado: (\d+)',log.read_bytes())
                    if len(usos)!=2 or usos[0]!=usos[1]:raise RuntimeError('La recarga consume heap')
                enviar(b'usuario\n');esperar(b'control devuelto al kernel.')
                enviar(b'help\n');esperar(b'help | version')
                print('NXE2 '+nombre+': correcto.')
            finally:p.terminate();p.wait(timeout=5)
        if disco.read_bytes()!=datos:raise RuntimeError('La imagen cambio')
finally:
    subprocess.run(ps+[str(raiz/'build.ps1'),'-Architecture','x86'],cwd=raiz,env=entorno,check=True,capture_output=True)
