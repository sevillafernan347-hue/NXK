"""Procesos cooperativos CPL3: reanudacion real, NEXTRY y regresiones de aislamiento."""
import pathlib,subprocess,sys,os,time,struct,re
sys.path.insert(0,str(pathlib.Path(__file__).resolve().parent))
from crear_volumen import crear
from empaquetar_usuario import empaquetar
raiz=pathlib.Path(__file__).resolve().parents[1]
carpeta=raiz/'build/x86/procesos-pruebas';carpeta.mkdir(exist_ok=True)
entorno={k:v for k,v in os.environ.items() if k.lower()!='psmodulepath'}
subprocess.run(['powershell.exe','-NoProfile','-ExecutionPolicy','Bypass','-File',str(raiz/'compilar-usuario.ps1'),
    '-Fuente','lenguaje/ejemplos/proceso.nxf','-Salida','build/x86/procesos-pruebas/proceso.nxe'],cwd=raiz,env=entorno,check=True,capture_output=True)
elf=bytearray(52);elf[:7]=b'\x7fELF\x01\x01\x01';struct.pack_into('<HHII',elf,16,2,3,1,0x400000)
def app(codigo):return empaquetar(elf,codigo)
# Comprobar todos los GPR de usuario, CF, DF y ESP tras dos cesiones.
c=bytearray();saltos=[]
def emitir(s):c.extend(bytes.fromhex(s))
def imm(op,n):c.extend(bytes.fromhex(op));c.extend(struct.pack('<I',n))
def salto(op):emitir(op);saltos.append(len(c));c.extend(bytes(4))
for op,n in [('bb',0x12345678),('b9',0x22334455),('ba',0x33445566),('be',0x44556677),('bf',0x55667788),('bd',0x66778899)]:imm(op,n)
imm('68',0x76543210);emitir('f9');imm('b8',9);emitir('cd 81');salto('0f 83')
emitir('85 c0');salto('0f 85')
for op,n in [('81 fb',0x12345678),('81 f9',0x22334455),('81 fa',0x33445566),('81 fe',0x44556677),('81 ff',0x55667788),('81 fd',0x66778899),('81 fc',0x401ffc)]:
    imm(op,n);salto('0f 85')
emitir('58');imm('3d',0x76543210);salto('0f 85')
emitir('fd');imm('b8',9);emitir('cd 81 9c 5a');imm('f7 c2',0x400);salto('0f 84')
emitir('fc');imm('b8',42);emitir('cd 80');fallo=len(c);emitir('0f 0b')
for pos in saltos:struct.pack_into('<i',c,pos,fallo-pos-4)
archivos={'proceso.nxe':(carpeta/'proceso.nxe').read_bytes(),'regs.nxe':app(c),
    'siempre.nxe':app(bytes.fromhex('b8 09 00 00 00 cd 81 eb f7')),
    'fpu.nxe':app(bytes.fromhex('db e3 31 c0 cd 80')),
    'segmentos.nxe':app(bytes.fromhex('31 c0 8e d8 8e c0 8e e0 8e e8 b8 09 00 00 00 cd 81 31 c0 cd 80')),
    'esp.nxe':app(bytes.fromhex('b8 09 00 00 00 bc 00 00 00 00 cd 81')),
    'ro.nxe':app(bytes.fromhex('c7 05 00 00 40 00 01 00 00 00 cd 80')),'bucle.nxe':app(b'\xeb\xfe')}
datos=crear(archivos);disco=carpeta/'procesos.img';disco.write_bytes(datos)
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
    def cargar(nombre):return int(re.search(rb'Proceso ID (\d+)',comando('proceso cargar '+nombre,b'Proceso ID '))[1])
    def paso(id,estado):return comando('proceso paso '+str(id),('Proceso '+str(id)+' estado '+str(estado)).encode())
    def liberar(id):comando('proceso liberar '+str(id),b'Proceso liberado.')
    def info(id,estado,turnos,cesiones):
        datos=comando('proceso listar')
        esperado=f'Proceso {id} estado {estado} turnos {turnos} cesiones {cesiones}'.encode()
        assert esperado in datos,(esperado,datos)
        return datos
    try:
        esperar(0);comando('volumen montar',b'Volumen NXFS montado')
        comando('proceso demo',b'Procesos cooperativos: registros y punto de ejecucion conservados.')
        comando('usuario nextry',b'Aplicacion NEXTRY protegida termino.')
        warm=cargar('proceso.nxe');comando('proceso ejecutar',b'Procesos sin pendientes.');liberar(warm)
        memoria=comando('mem')
        a=cargar('proceso.nxe');b=cargar('proceso.nxe');r=cargar('regs.nxe');malo=cargar('ro.nxe')
        ocupada=comando('mem');comando('proceso cargar bucle.nxe',b'No se pudo cargar proceso.');assert comando('mem')==ocupada
        comando('proceso demo',b'No se pudo verificar procesos.');assert comando('mem')==ocupada
        paso(a,1);paso(b,1);paso(r,1);paso(malo,4)
        comando('usuario turno '+str(a),b'No se pudo ejecutar el turno.')
        paso(a,1);paso(b,1);paso(r,1)
        fin=paso(a,3);assert b'Proceso NEXTRY: terminado' in fin and b'42' in fin
        paso(b,3);fin=paso(r,3);assert b'normal, salida 42' in fin
        info(a,3,3,2);info(b,3,3,2);info(r,3,3,2)
        for id in [a,b,r,malo]:liberar(id)
        assert comando('mem')==memoria
        comando('proceso paso '+str(a),b'No se pudo ejecutar proceso.')
        # FPU bloqueada, segmentos nulos reanudados y contexto de pila invalido contenido.
        for archivo,vector in [('fpu.nxe',7)]:
            id=cargar(archivo);fin=paso(id,4);assert ('fallo, vector '+str(vector)).encode() in fin
            liberar(id)
        id=cargar('bucle.nxe');paso(id,1);assert b'preempciones 1' in info(id,1,1,0);liberar(id)
        id=cargar('segmentos.nxe');paso(id,1);paso(id,3);liberar(id)
        id=cargar('esp.nxe');comando('proceso paso '+str(id),b'No se pudo ejecutar proceso.');assert b'Error de motor 6' in info(id,4,1,0);liberar(id)
        id=cargar('siempre.nxe');comando('proceso ejecutar',b'Presupuesto agotado; quedan procesos listos.');info(id,1,32,32)
        # Liberacion por API anterior invalida el PCB y no resucita al reciclar slot.
        comando('usuario liberar '+str(id),b'Programa privado liberado.');comando('proceso listar',b'Sin procesos registrados.')
        nuevo=cargar('proceso.nxe');assert nuevo!=id
        comando('proceso paso '+str(id),b'No se pudo ejecutar proceso.')
        comando('proceso ejecutar',b'Procesos sin pendientes.');info(nuevo,3,3,2);liberar(nuevo)
        for _ in range(3):comando('proceso demo',b'Procesos cooperativos: registros y punto de ejecucion conservados.')
        assert comando('mem')==memoria
        # El runtime anterior rechaza ceder explicitamente, sin quedar bloqueado.
        antiguo=comando('ejecutar usuario proceso.nxe',b'No se pudo cargar o ejecutar NXE2.')
        comando('usuario nextry',b'Aplicacion NEXTRY protegida termino.')
        comando('tareas ocho',b'Ocho tareas: argumentos, pilas, esperas y finalizacion verificadas.')
        comando('help',b'proceso ejecutar')
    finally:p.terminate();p.wait(timeout=5)
assert disco.read_bytes()==datos and arranque.read_bytes()==original
print('PROCESOS OK: reanudacion GPR/CF/DF/ESP, NEXTRY, rondas, fallos y memoria estable.')
