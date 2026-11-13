"""Ejercita las barreras reales del linker sin arrancar ni tocar discos.

Las variables NOLOAD pueden crecer mas que el archivo cargado por BIOS, pero
ninguna de ambas regiones debe poder exceder su limite por accidente.
"""
import subprocess
import tempfile
from pathlib import Path

RAIZ=Path(__file__).resolve().parents[1]
ZIG=RAIZ/'tools/zig/zig.exe'

def main():
    casos=(
        ('bss_ampliada','.section .bss\n.skip 0x21000\n',True,None),
        ('bss_excesiva','.section .bss\n.skip 0x30000\n',False,'192 KiB'),
        ('imagen_en_limite','.skip 0x1dfff\n',True,None),
        ('imagen_excesiva','.skip 0x1e000\n',False,'120 KiB'),
        ('datos_excesivos','.section .data\n.skip 0x1e000\n',False,'120 KiB'),
    )
    carpeta=RAIZ/'build'; carpeta.mkdir(exist_ok=True)
    with tempfile.TemporaryDirectory(prefix='limites-kernel-',dir=carpeta) as temporal:
        base=Path(temporal)
        for nombre,contenido,exito,mensaje in casos:
            fuente=base/(nombre+'.S'); objeto=base/(nombre+'.o')
            fuente.write_text('.section .text.entry\n.global kernel_entry\nkernel_entry:\nret\n'+contenido)
            subprocess.run([str(ZIG),'cc','-target','x86-freestanding-none','-c',
                str(fuente),'-o',str(objeto)],check=True,capture_output=True)
            r=subprocess.run([str(ZIG),'ld.lld','-T',str(RAIZ/'kernel/linker.ld'),
                str(objeto),'-o',str(base/(nombre+'.elf'))],capture_output=True,text=True)
            assert (r.returncode==0)==exito,(nombre,r.stdout,r.stderr)
            if mensaje:assert mensaje in r.stderr,(nombre,r.stderr)
    print('LIMITES OK: BSS ampliada valida, barreras192KiB RAM y120KiB archivo activas.')

if __name__=='__main__':main()
