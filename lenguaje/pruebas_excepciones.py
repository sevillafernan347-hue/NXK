"""Provoca INT3, instruccion invalida y selector invalido en QEMU x86.
Comprueba vector, error y direccion; restaura la imagen normal al finalizar.
"""
import os, pathlib, re, subprocess
entorno=os.environ.copy()
# Windows PowerShell debe reconstruir sus rutas, no heredar las de PowerShell 7.
entorno={k:v for k,v in entorno.items() if k.lower()!='psmodulepath'}
raiz=pathlib.Path(__file__).resolve().parents[1]
construir=['powershell.exe','-NoProfile','-ExecutionPolicy','Bypass','-File',str(raiz/'build.ps1'),'-Architecture','x86']
try:
    for vector,error in ((3,0),(6,0),(13,0xfff8),(14,2),(114,3),(214,2)):
        subprocess.run(construir+['-Test','-ExceptionTest','-ExceptionVector',str(vector)],check=True,env=entorno)
        p=subprocess.run([str(raiz/'tools/qemu/qemu-system-i386.exe'),'-machine','pc','-accel','tcg',
            '-cpu','qemu32','-m','32M','-drive','file=build/x86/nxk-test.img,format=raw,if=ide',
            '-display','none','-serial','stdio','-monitor','none','-no-reboot',
            '-device','isa-debug-exit,iobase=0xf4,iosize=0x04'],cwd=raiz,capture_output=True,timeout=30)
        (raiz/f'build/x86/excepcion-{vector}.log').write_bytes(p.stdout)
        m=re.search(rb'VECTOR: 0x([0-9A-F]+) ERROR: 0x([0-9A-F]+) EIP: 0x([0-9A-F]+)',p.stdout)
        if p.returncode!=37 or not m or tuple(int(x,16) for x in m.groups()[:2])!=(14 if vector in (114,214) else vector,error):
            raise RuntimeError(f'Diagnostico incorrecto para vector {vector}: {p.stdout!r}')
        if not 0x10000<=int(m[3],16)<0x18000: raise RuntimeError('EIP fuera del kernel')
        if vector==14 and b'CR2: 0x00000000' not in p.stdout: raise RuntimeError('CR2 nulo incorrecto')
        if vector==214 and b'CR2: 0x00101000' not in p.stdout: raise RuntimeError('Guarda no detectada')
        if vector==114:
            cr2=re.search(rb'CR2: 0x([0-9A-F]+)',p.stdout)
            if not cr2 or not 0x10000<=int(cr2[1],16)<0x18000: raise RuntimeError('CR2 fuera de codigo')
        print(f'EXCEPCION {vector}: vector, error y EIP verificados.')
finally:
    subprocess.run(construir,check=True,env=entorno)
