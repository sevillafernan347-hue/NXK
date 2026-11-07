"""NXE2 retenidos en espacios CPL3: turnos, datos, fallos, limites y liberacion."""
import pathlib,subprocess,sys,os,time,struct,re
sys.path.insert(0,str(pathlib.Path(__file__).resolve().parent))
from crear_volumen import crear
from empaquetar_usuario import empaquetar
raiz=pathlib.Path(__file__).resolve().parents[1]
carpeta=raiz/'build/x86/ejecucion-privada-pruebas';carpeta.mkdir(exist_ok=True)
entorno={k:v for k,v in os.environ.items() if k.lower()!='psmodulepath'}
subprocess.run(['powershell.exe','-NoProfile','-ExecutionPolicy','Bypass','-File',str(raiz/'compilar-usuario.ps1'),
    '-Fuente','lenguaje/ejemplos/usuario_disco.nxf','-Salida','build/x86/ejecucion-privada-pruebas/nextry.nxe'],
    cwd=raiz,env=entorno,check=True,capture_output=True)
elf=bytearray(52);elf[:7]=b'\x7fELF\x01\x01\x01';struct.pack_into('<HHII',elf,16,2,3,1,0x400000)
def app(codigo):return empaquetar(elf,codigo)
a=bytes.fromhex('a1 00 10 40 00 83 c0 01 a3 00 10 40 00 cd 80')
b=bytes.fromhex('a1 00 10 40 00 83 c0 0a a3 00 10 40 00 cd 80')
archivos={'a.nxe':app(a),'b.nxe':app(b),'ro.nxe':app(bytes.fromhex('c7 05 00 00 40 00 01 00 00 00 cd 80')),
    'loop.nxe':app(b'\xeb\xfe'),'salida.nxe':app(bytes.fromhex('b8 20 00 00 80 cd 80')),
    'malo.nxe':b'X'*64,'nextry.nxe':(carpeta/'nextry.nxe').read_bytes(),'leeme.txt':b'Datos para app retenida\n'}
disco=carpeta/'apps.img';datos=crear(archivos);disco.write_bytes(datos)
arranque=raiz/'build/x86/nxk.img';original=arranque.read_bytes();log=carpeta/'consola.log'
with log.open('wb',buffering=0) as salida:
    p=subprocess.Popen([str(raiz/'tools/qemu/qemu-system-i386.exe'),'-machine','pc','-accel','tcg','-cpu','qemu32','-m','32M',
        '-drive',f'file={arranque.as_posix()},format=raw,if=ide,index=0,snapshot=on',
        '-drive',f'file={disco.as_posix()},format=raw,if=ide,index=1,snapshot=on',
        '-display','none','-serial','stdio','-monitor','none','-no-reboot'],cwd=raiz,
        stdin=subprocess.PIPE,stdout=salida,stderr=salida)
    def esperar(desde):
        fin=time.monotonic()+20
        while time.monotonic()<fin:
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
    def cargar(nombre):
        d=comando('usuario cargar '+nombre,b'Programa privado ID ')
        return int(re.search(rb'Programa privado ID (\d+)',d)[1])
    def turno(id,valor):comando('usuario turno '+str(id),('normal, salida '+str(valor)).encode())
    def liberar(id):comando('usuario liberar '+str(id),b'Programa privado liberado.')
    try:
        esperar(0)
        comando('volumen montar',b'Volumen NXFS montado')
        comando('usuario espacios',b'Programas privados: datos independientes, fallo y bucle contenidos.')
        comando('usuario nextry',b'Aplicacion NEXTRY protegida termino.')
        # Calentar los buffers de lectura/FS. Las cargas posteriores no crecen heap.
        id=cargar('nextry.nxe');turno(id,0);liberar(id)
        memoria=comando('mem');historial=comando('usuario historial')
        comando('usuario cargar malo.nxe',b'No se pudo preparar NXE2 privado.')
        comando('usuario cargar ausente.nxe',b'No se pudo preparar NXE2 privado.')
        assert comando('mem')==memoria
        assert comando('usuario historial')==historial
        for invalido in ('0','-1','1x','4294967296','4294967295'):
            comando('usuario turno '+invalido,b'No se pudo ejecutar el turno.')
            comando('usuario liberar '+invalido,b'No se pudo liberar programa.')
        ids=[cargar(n) for n in ['a.nxe','b.nxe','ro.nxe','loop.nxe']]
        assert len(set(ids))==4
        lista=comando('usuario listar')
        assert set(map(int,re.findall(rb'Programa privado ID (\d+)',lista)))==set(ids)
        ocupada=comando('mem');comando('usuario cargar salida.nxe',b'No se pudo preparar NXE2 privado.')
        assert comando('mem')==ocupada
        comando('usuario espacios',b'No se pudo verificar ejecucion privada.')
        assert comando('mem')==ocupada
        turno(ids[0],1);turno(ids[1],10);turno(ids[0],2)
        comando('usuario turno '+str(ids[2]),b'fallo, vector 14 error 7')
        comando('usuario turno '+str(ids[3]),b'fallo, vector 32')
        turno(ids[0],3);turno(ids[1],20)
        for id in ids:liberar(id)
        comando('usuario listar',b'Sin programas privados cargados.')
        assert comando('mem')==memoria
        for id in ids:comando('usuario turno '+str(id),b'No se pudo ejecutar el turno.')
        alto=cargar('salida.nxe');assert alto not in ids;turno(alto,0x80000020);liberar(alto)
        for _ in range(3):
            nuevo=cargar('nextry.nxe');turno(nuevo,0);turno(nuevo,0);liberar(nuevo)
            assert comando('mem')==memoria
        comando('usuario nextry',b'Aplicacion NEXTRY protegida termino.')
        comando('tareas ocho',b'Ocho tareas: argumentos, pilas, esperas y finalizacion verificadas.')
        comando('help',b'usuario turno ID')
    finally:p.terminate();p.wait(timeout=5)
assert disco.read_bytes()==datos and arranque.read_bytes()==original
print('EJECUCION PRIVADA OK: cuatro NXE2, turnos CPL3, datos separados, fallos, NEXTRY y memoria estable.')
