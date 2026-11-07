"""Preempcion CPL3 en QEMU: bucle no cooperativo y progreso de otros procesos."""
import pathlib,subprocess,sys,os,time,struct,re
sys.path.insert(0,str(pathlib.Path(__file__).resolve().parent))
from crear_volumen import crear
from empaquetar_usuario import empaquetar
raiz=pathlib.Path(__file__).resolve().parents[1]
carpeta=raiz/'build/x86/preempcion-pruebas';carpeta.mkdir(exist_ok=True)
entorno={k:v for k,v in os.environ.items() if k.lower()!='psmodulepath'}
subprocess.run(['powershell.exe','-NoProfile','-ExecutionPolicy','Bypass','-File',str(raiz/'compilar-usuario.ps1'),
    '-Fuente','lenguaje/ejemplos/proceso.nxf','-Salida','build/x86/preempcion-pruebas/proceso.nxe'],cwd=raiz,env=entorno,check=True,capture_output=True)
subprocess.run(['powershell.exe','-NoProfile','-ExecutionPolicy','Bypass','-File',str(raiz/'compilar-usuario.ps1'),
    '-Fuente','lenguaje/ejemplos/proceso_ocupado.nxf','-Salida','build/x86/preempcion-pruebas/ocupado.nxe'],cwd=raiz,env=entorno,check=True,capture_output=True)
elf=bytearray(52);elf[:7]=b'\x7fELF\x01\x01\x01';struct.pack_into('<HHII',elf,16,2,3,1,0x400000)
def app(c):return empaquetar(elf,c)
datos=crear({'bucle.nxe':(carpeta/'ocupado.nxe').read_bytes(),
    'nulo.nxe':app(bytes.fromhex('31 c0 8e d8 8e c0 8e e0 8e e8 eb fe')),'proceso.nxe':(carpeta/'proceso.nxe').read_bytes(),
    'fin.nxe':app(bytes.fromhex('b8 07 00 00 00 cd 80')),'fallo.nxe':app(bytes.fromhex('0f 0b'))})
disco=carpeta/'preempcion.img';disco.write_bytes(datos)
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
        for b in (c+'\n').encode():p.stdin.write(bytes([b]));p.stdin.flush();time.sleep(.003)
        d=esperar(desde)
        if esperado is not None:assert esperado in d,(c,d)
        return d
    def cargar(nombre):return int(re.search(rb'Proceso ID (\d+)',comando('proceso cargar '+nombre,b'Proceso ID '))[1])
    def estado():
        d=comando('proceso listar')
        return {int(id):tuple(map(int,(e,t,c,p))) for id,e,t,c,p in re.findall(rb'Proceso (\d+) estado (\d+) turnos (\d+) cesiones (\d+) preempciones (\d+)',d)}
    def liberar(id):comando('proceso liberar '+str(id),b'Proceso liberado.')
    try:
        esperar(0);comando('volumen montar',b'Volumen NXFS montado')
        comando('proceso preempcion',b'Preempcion: bucle suspendido, otro proceso completo y registros conservados.')
        w=cargar('fin.nxe');comando('proceso ejecutar',b'Procesos sin pendientes.');liberar(w)
        memoria=comando('mem')
        a,b,c,d=[cargar(n) for n in ['bucle.nxe','proceso.nxe','fin.nxe','fallo.nxe']]
        ocupada=comando('mem')
        comando('proceso preempcion',b'No se pudo verificar preempcion.')
        assert comando('mem')==ocupada
        resultado=comando('proceso ejecutar',b'Presupuesto agotado; quedan procesos listos.')
        assert b'Proceso NEXTRY: terminado' in resultado and b'42' in resultado
        estados=estado();e,t,cesiones,preempciones=estados[a]
        assert e==1 and t==preempciones and preempciones>0 and cesiones==0,estados
        assert estados[b][0]==3 and estados[b][2]==2 and estados[c][0]==3 and estados[d][0]==4,estados
        for id in [b,c,d]:liberar(id)
        antes=comando('mem')
        comando('proceso ejecutar',b'Presupuesto agotado; quedan procesos listos.')
        assert estado()[a]==(1,t+32,0,preempciones+32)
        assert comando('mem')==antes
        comando('usuario turno '+str(a),b'No se pudo ejecutar el turno.')
        liberar(a);comando('proceso ejecutar',b'Procesos sin pendientes.')
        assert comando('mem')==memoria
        nulo=cargar('nulo.nxe')
        comando('proceso paso '+str(nulo),('Proceso '+str(nulo)+' estado 1').encode())
        comando('proceso paso '+str(nulo),('Proceso '+str(nulo)+' estado 1').encode())
        assert estado()[nulo]==(1,2,0,2)
        liberar(nulo)
        for _ in range(3):comando('proceso preempcion',b'Preempcion: bucle suspendido, otro proceso completo y registros conservados.')
        assert comando('mem')==memoria
        # La ruta sincronica previa conserva su politica terminal de bucle.
        comando('usuario bucle',b'Bucle interrumpido por PIT; consola recuperada.')
        comando('usuario estado',b'fallo, vector 32')
        comando('usuario nextry',b'Aplicacion NEXTRY protegida termino.')
        comando('tareas ocho',b'Ocho tareas: argumentos, pilas, esperas y finalizacion verificadas.')
        comando('help',b'proceso preempcion')
    finally:p.terminate();p.wait(timeout=5)
assert disco.read_bytes()==datos and arranque.read_bytes()==original
print('PREEMPCION OK: bucle no bloquea NEXTRY ni otras apps, rondas acotadas y recursos estables.')
