"""Persistencia en discos virtuales descartables: reinicio, corrupcion y commits cortados."""
import pathlib,subprocess,time,struct,sys
sys.path.insert(0,str(pathlib.Path(__file__).resolve().parent))
from crear_persistencia import crear,huella
raiz=pathlib.Path(__file__).resolve().parents[1]
carpeta=raiz/'build/x86/persistencia-pruebas'; carpeta.mkdir(exist_ok=True)
arranque=raiz/'build/x86/nxk.img'; original=arranque.read_bytes()
numero=0
class Maquina:
    def __init__(self,disco):self.disco=disco
    def __enter__(self):
        global numero
        numero+=1; self.log=carpeta/f'caso-{numero}.log'; self.f=self.log.open('wb',buffering=0)
        self.p=subprocess.Popen([str(raiz/'tools/qemu/qemu-system-i386.exe'),'-machine','pc','-accel','tcg','-cpu','qemu32','-m','32M',
            '-drive',f'file={arranque.as_posix()},format=raw,if=ide,index=0,snapshot=on',
            '-drive',f'file={self.disco.as_posix()},format=raw,if=ide,index=1,cache=writeback',
            '-display','none','-serial','stdio','-monitor','none','-no-reboot'],cwd=raiz,
            stdin=subprocess.PIPE,stdout=self.f,stderr=self.f)
        try:self.esperar(0)
        except BaseException:self.__exit__(None,None,None);raise
        return self
    def esperar(self,desde):
        fin=time.monotonic()+15
        while time.monotonic()<fin:
            datos=self.log.read_bytes()[desde:]
            if b'nxk> ' in datos:return datos
            if self.p.poll() is not None:break
            time.sleep(.02)
        raise AssertionError(self.log.read_text(errors='replace'))
    def comando(self,texto,esperado):
        desde=self.log.stat().st_size
        for v in (texto+'\n').encode():self.p.stdin.write(bytes([v]));self.p.stdin.flush();time.sleep(.003)
        datos=self.esperar(desde)
        assert esperado in datos,(texto,datos)
        return datos
    def __exit__(self,*args):
        self.p.terminate();self.p.wait(timeout=5);self.f.close()

def imagen(nombre,datos):
    p=carpeta/(nombre+'.img');p.write_bytes(datos);return p

def comprobar(nombre,datos,esperado=b'ANTERIOR_0',fallo=False):
    p=imagen(nombre,datos)
    with Maquina(p) as m:
        if fallo:
            m.comando('escribir testigo CONSERVADO',b'Archivo guardado')
            m.comando('persistencia cargar',b'Persistencia fallo')
            m.comando('leer testigo',b'CONSERVADO')
            m.comando('persistencia guardar',b'Persistencia fallo')
        else:
            m.comando('persistencia cargar',b'Instantanea recuperada')
            m.comando('leer f0',esperado)
    assert p.read_bytes()==datos,nombre+' modifico disco al rechazar/leer'
    print('PERSISTENCIA '+nombre+': OK',flush=True)

p=imagen('reinicios',crear())
with Maquina(p) as m:
    m.comando('persistencia cargar',b'Disco de datos sin instantanea')
    for i in range(8):m.comando(f'escribir f{i} ANTERIOR_{i}',b'Archivo guardado')
    m.comando('persistencia guardar',b'Instantanea guardada')
antigua=p.read_bytes()
with Maquina(p) as m:
    m.comando('persistencia cargar',b'Instantanea recuperada')
    for i in range(8):
        m.comando(f'leer f{i}',f'ANTERIOR_{i}'.encode())
        m.comando(f'escribir f{i} NUEVO_{i}',b'Archivo guardado')
    m.comando('persistencia guardar',b'Instantanea guardada')
nueva=p.read_bytes()
assert struct.unpack_from('<I',antigua,512+8)[0]==1
assert struct.unpack_from('<I',nueva,5120+8)[0]==2
with Maquina(p) as m:
    m.comando('persistencia cargar',b'Instantanea recuperada')
    for i in range(8):m.comando(f'leer f{i}',f'NUEVO_{i}'.encode())
    antes=m.comando('mem',b'RAM')
    for _ in range(3):m.comando('persistencia cargar',b'Instantanea recuperada')
    assert m.comando('mem',b'RAM')==antes
    for i in range(8):m.comando(f'borrar f{i}',b'Archivo borrado')
    m.comando('persistencia guardar',b'Instantanea guardada')
with Maquina(p) as m:
    m.comando('escribir temporal DESCARTAR',b'Archivo guardado')
    m.comando('persistencia cargar',b'Instantanea recuperada')
    m.comando('leer f0',b'No se pudo leer')
    m.comando('leer temporal',b'No se pudo leer')
tercera=p.read_bytes()
print('PERSISTENCIA reinicios, 8 archivos, vacio y memoria estable: OK',flush=True)
# Interrumpir el segundo guardado tras 0..8 sectores conserva el primero.
for n in range(9):
    b=bytearray(antigua);b[11*512:(11+n)*512]=nueva[11*512:(11+n)*512]
    comprobar('corte-'+str(n),b)
comprobar('commit-completo',nueva,b'NUEVO_0')
# La tercera escritura reutiliza banco0: su vieja cabecera NO es un commit nuevo.
for n in range(9):
    b=bytearray(nueva);b[2*512:(2+n)*512]=tercera[2*512:(2+n)*512]
    comprobar('reutilizacion-corte-'+str(n),b,b'NUEVO_0')
for n in [1,16,256,510]:
    b=bytearray(nueva);b[2*512:10*512]=tercera[2*512:10*512]
    b[512:512+n]=tercera[512:512+n]
    comprobar('cabecera-parcial-'+str(n),b,b'NUEVO_0')

for nombre,pos in [('cabecera',10*512),('datos',11*512+32)]:
    b=bytearray(nueva);b[pos]^=1;comprobar(nombre,b)
# Metadatos maliciosos con checksums recalculados tampoco entran a RAM.
for nombre in ['longitud','duplicado','relleno','nombre']:
    b=bytearray(nueva)
    if nombre=='longitud':struct.pack_into('<I',b,11*512+24,257)
    if nombre=='duplicado':b[12*512:12*512+24]=b[11*512:11*512+24]
    if nombre=='relleno':b[11*512+511]=1
    if nombre=='nombre':b[11*512:11*512+24]=b'x'*24
    struct.pack_into('<I',b,10*512+16,huella(b[11*512:19*512]))
    struct.pack_into('<I',b,10*512+508,huella(b[10*512:10*512+508]))
    comprobar(nombre,b)
b=bytearray(nueva);b[512]^=1;b[5120]^=1;comprobar('ambos-invalidos',b,fallo=True)
b=bytearray(nueva);b[24]=1;struct.pack_into('<I',b,508,huella(b[:508]));comprobar('superbloque',b,fallo=True)
b=bytearray(nueva);struct.pack_into('<I',b,5120+8,1);struct.pack_into('<I',b,5120+508,huella(b[5120:5628]));comprobar('generacion-ambigua',b,fallo=True)
b=bytearray(crear());b[1024:1536]=antigua[1024:1536];comprobar('primer-commit-cortado',b,fallo=True)
comprobar('sin-formato',bytes(9728),fallo=True)
from crear_volumen import crear as crear_nxfs
comprobar('nxfs-no-se-escribe',crear_nxfs({'leeme':b'ORIGINAL'}),fallo=True)
b=bytearray(nueva);struct.pack_into('<I',b,5120+8,0xffffffff);struct.pack_into('<I',b,5120+508,huella(b[5120:5628]))
p=imagen('generacion-agotada',b)
with Maquina(p) as m:
    m.comando('persistencia cargar',b'Instantanea recuperada')
    m.comando('leer f0',b'NUEVO_0')
    m.comando('persistencia guardar',b'Persistencia fallo; codigo 5')
assert p.read_bytes()==b
# Archivo binario completo, incluyendo NUL, debe sobrevivir importacion/exportacion.
b=bytearray(antigua); contenido=bytes(range(256))
struct.pack_into('<I',b,1024+24,256); b[1024+32:1024+288]=contenido
struct.pack_into('<I',b,512+16,huella(b[1024:5120]))
struct.pack_into('<I',b,512+508,huella(b[512:1020]))
p=imagen('binario',b)
with Maquina(p) as m:
    m.comando('persistencia cargar',b'Instantanea recuperada')
    m.comando('persistencia guardar',b'Instantanea guardada')
assert p.read_bytes()[11*512+32:11*512+288]==contenido
print('PERSISTENCIA binario de 256 bytes: OK',flush=True)
assert arranque.read_bytes()==original,'Se altero disco de arranque'
print('PERSISTENCIA generacion agotada y disco de arranque intacto: OK',flush=True)
