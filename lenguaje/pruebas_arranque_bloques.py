"""Comprueba lectura BIOS completa de 120 KiB con nuestro arranque NXA."""
import pathlib,struct,subprocess,sys
raiz=pathlib.Path(__file__).resolve().parents[1]
sys.path.insert(0,str(pathlib.Path(__file__).resolve().parent))
from ensamblador import ensamblar
carpeta=raiz/'build/x86/arranque-bloques';carpeta.mkdir(exist_ok=True)
boot=(raiz/(sys.argv[1] if len(sys.argv)>1 else 'build/x86/boot.bin')).read_bytes()
if len(boot)!=512 or boot[-2:]!=b'\x55\xaa':raise RuntimeError('Boot invalido')
marcas=[(0x17ffc,0x11223344),(0x18000,0x55667788),(0x1fffc,0x10203040),(0x20000,0x50607080),(0x2dffc,0x99aabbcc)]
lineas=['origen 0x10000','modo 32']
for direccion,valor in marcas:
    lineas += [f'leer_memoria32 eax, {direccion}',f'comparar_inmediato32 eax, {valor}','si_distinto fallo']
lineas += ['cargar8 al, 0x10','salida8 0xf4','detener','fallo:','cargar8 al, 0x11','salida8 0xf4','detener']
codigo=ensamblar('\n'.join(lineas))
for nombre,corromper in [('completo',None),('fin_primero',0),('inicio_segundo',1),('fin_segundo',2),('inicio_tercero',3),('fin_tercero',4)]:
    carga=bytearray([0xcc])*122880;carga[:len(codigo)]=codigo
    for i,(direccion,valor) in enumerate(marcas):
        struct.pack_into('<I',carga,direccion-0x10000,valor^(1 if i==corromper else 0))
    disco=bytearray(1024*1024);disco[:512]=boot;disco[512:512+len(carga)]=carga
    imagen=carpeta/f'{nombre}.img';imagen.write_bytes(disco)
    p=subprocess.run([str(raiz/'tools/qemu/qemu-system-i386.exe'),'-machine','pc','-accel','tcg','-cpu','qemu32',
        '-m','32M','-drive',f'file={imagen.as_posix()},format=raw,if=ide','-snapshot','-display','none',
        '-serial','none','-monitor','none','-no-reboot','-device','isa-debug-exit,iobase=0xf4,iosize=0x04'],
        cwd=raiz,capture_output=True,timeout=15)
    if p.returncode!=(33 if corromper is None else 35):raise RuntimeError(nombre+': '+str(p.returncode)+p.stderr.decode(errors='replace'))
    if imagen.read_bytes()!=disco:raise RuntimeError('Cambio la imagen base')
    print('ARRANQUE 120 KiB '+nombre+': correcto.')
