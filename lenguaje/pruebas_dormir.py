"""Procesos en espera: no consumen turnos, reanudan al vencer y conservan recursos."""
import pathlib,subprocess,sys,os,time,struct,re
sys.path.insert(0,str(pathlib.Path(__file__).resolve().parent))
from crear_volumen import crear
from empaquetar_usuario import empaquetar
raiz=pathlib.Path(__file__).resolve().parents[1]
carpeta=raiz/'build/x86/dormir-pruebas';carpeta.mkdir(exist_ok=True)
entorno={k:v for k,v in os.environ.items() if k.lower()!='psmodulepath'}
subprocess.run(['powershell.exe','-NoProfile','-ExecutionPolicy','Bypass','-File',str(raiz/'compilar-usuario.ps1'),
    '-Fuente','lenguaje/ejemplos/proceso_dormir.nxf','-Salida','build/x86/dormir-pruebas/dormir.nxe'],cwd=raiz,env=entorno,check=True,capture_output=True)
elf=bytearray(52);elf[:7]=b'\x7fELF\x01\x01\x01';struct.pack_into('<HHII',elf,16,2,3,1,0x400000)
def app(c):return empaquetar(elf,c)
def espera(ticks):return app(b'\xbb'+struct.pack('<I',ticks)+bytes.fromhex('b8 0a 00 00 00 cd 81 cd 80'))
datos=crear({'dormir.nxe':(carpeta/'dormir.nxe').read_bytes(),'largo.nxe':espera(0x7fffffff),
    'cero.nxe':espera(0),'invalido.nxe':espera(0x80000000),'maxerr.nxe':espera(0xffffffff),
    'breve.nxe':espera(30),'fin.nxe':app(bytes.fromhex('b8 07 00 00 00 cd 80')),'malo.nxe':b'X'*64})
disco=carpeta/'dormir.img';disco.write_bytes(datos)
arranque=raiz/'build/x86/nxk.img';original=arranque.read_bytes();log=carpeta/'consola.log'
with log.open('wb',buffering=0) as salida:
    p=subprocess.Popen([str(raiz/'tools/qemu/qemu-system-i386.exe'),'-machine','pc','-accel','tcg','-cpu','qemu32','-m','32M',
        '-drive',f'file={arranque.as_posix()},format=raw,if=ide,index=0,snapshot=on',
        '-drive',f'file={disco.as_posix()},format=raw,if=ide,index=1,snapshot=on',
        '-display','none','-serial','stdio','-monitor','none','-no-reboot'],cwd=raiz,
        stdin=subprocess.PIPE,stdout=salida,stderr=salida)
    def esperar(desde):
        limite=time.monotonic()+20
        while time.monotonic()<limite:
            d=log.read_bytes()[desde:]
            if b'nxk> ' in d:return d
            if p.poll() is not None:break
            time.sleep(.02)
        raise AssertionError(log.read_text(errors='replace'))
    def comando(c,esperado=None):
        desde=log.stat().st_size
        for v in (c+'\n').encode():p.stdin.write(bytes([v]));p.stdin.flush();time.sleep(.003)
        d=esperar(desde)
        if esperado is not None:assert esperado in d,(c,d)
        return d
    def cargar(nombre):return int(re.search(rb'Proceso ID (\d+)',comando('proceso cargar '+nombre,b'Proceso ID '))[1])
    def liberar(id):comando('proceso liberar '+str(id),b'Proceso liberado.')
    def estados():
        d=comando('proceso listar')
        filas=re.findall(rb'Proceso (\d+) estado (\d+) turnos (\d+) cesiones (\d+) preempciones (\d+) dormidas (\d+) despertar (\d+)',d)
        return {int(id):tuple(map(int,(e,t,c,p,s,w))) for id,e,t,c,p,s,w in filas},d
    def completar_dormir(id):
        fin=time.monotonic()+5; texto=bytearray()
        while time.monotonic()<fin:
            d=comando('proceso ejecutar');texto.extend(d)
            if b'Procesos sin pendientes.' in d:break
            assert b'Procesos esperando plazo' in d or b'Presupuesto agotado' in d,d
            time.sleep(.03)
        else:raise AssertionError('No reanudo al vencer')
        info,d=estados();e,t,c,p,s,w=info[id]
        assert e==3 and t>=3 and c==1 and s==1 and w==0,(info,d)
        assert b'normal, salida 0' in d and b'Proceso NEXTRY: espera terminada' in texto
    try:
        esperar(0);comando('volumen montar',b'Volumen NXFS montado')
        comando('proceso dormir',b'Espera de proceso: otro avanzo y el dormido continuo al vencer.')
        comando('usuario nextry',b'Aplicacion NEXTRY protegida termino.')
        id=cargar('dormir.nxe');completar_dormir(id);liberar(id)
        memoria=comando('mem')
        a=cargar('largo.nxe');b=cargar('fin.nxe')
        comando('proceso ejecutar',b'Procesos esperando plazo')
        info,_=estados();assert info[a][:5]==(5,1,0,0,1) and info[b][0]==3,info
        ocupada=comando('mem')
        for _ in range(3):
            comando('proceso paso '+str(a),b'No se pudo ejecutar proceso.')
            comando('proceso ejecutar',b'Procesos esperando plazo')
        assert estados()[0][a]==info[a] and comando('mem')==ocupada
        comando('proceso dormir',b'No se pudo verificar espera.')
        assert comando('mem')==ocupada
        z=cargar('cero.nxe');comando('proceso paso '+str(z),('Proceso '+str(z)+' estado 1').encode())
        assert estados()[0][z][2:]==(1,0,0,0)
        comando('proceso paso '+str(z),b'normal, salida 0');liberar(z)
        for archivo in ['invalido.nxe','maxerr.nxe']:
            id=cargar(archivo);comando('proceso paso '+str(id),b'normal, salida 4294967295')
            assert estados()[0][id][2:]==(0,0,0,0);liberar(id)
        liberar(a);liberar(b);assert comando('mem')==memoria
        comando('proceso paso '+str(a),b'No se pudo ejecutar proceso.')
        # Borrar un dormido no deja un despertar sobre la nueva generacion.
        viejo=cargar('breve.nxe');comando('proceso paso '+str(viejo))
        liberar(viejo);nuevo=cargar('fin.nxe');assert nuevo!=viejo
        comando('proceso paso '+str(nuevo),b'normal, salida 7');time.sleep(.5)
        comando('proceso ejecutar',b'Procesos sin pendientes.')
        assert estados()[0][nuevo]==(3,1,0,0,0,0);liberar(nuevo)
        for _ in range(3):
            id=cargar('dormir.nxe');completar_dormir(id);liberar(id)
        assert comando('mem')==memoria
        historial=comando('usuario historial');comando('proceso cargar malo.nxe',b'No se pudo cargar proceso.')
        assert comando('mem')==memoria and comando('usuario historial')==historial
        comando('ejecutar usuario dormir.nxe',b'No se pudo cargar o ejecutar NXE2.')
        comando('usuario nextry',b'Aplicacion NEXTRY protegida termino.')
        comando('tareas ocho',b'Ocho tareas: argumentos, pilas, esperas y finalizacion verificadas.')
        comando('proceso preempcion',b'Preempcion: bucle suspendido, otro proceso completo y registros conservados.')
        comando('help',b'proceso dormir')
    finally:p.terminate();p.wait(timeout=5)
assert disco.read_bytes()==datos and arranque.read_bytes()==original
print('DORMIR OK: espera sin turnos, NEXTRY reanudado, limites, reciclaje y memoria estable.')
