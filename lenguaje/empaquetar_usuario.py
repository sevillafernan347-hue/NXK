"""NXE version 2: imagen x86 CPL3 para servicios INT 0x81, cabecera de 64 bytes."""
import argparse,struct
from pathlib import Path
def huella(datos):
    h=2166136261
    for b in datos:h=((h^b)*16777619)&0xffffffff
    return h
def empaquetar(elf,binario):
    if len(elf)<52 or elf[:7]!=b'\x7fELF\x01\x01\x01' or struct.unpack_from('<H',elf,16)[0]!=2 or struct.unpack_from('<I',elf,20)[0]!=1 or struct.unpack_from('<H',elf,18)[0]!=3:raise ValueError('Se requiere ELF32 x86 little-endian')
    if struct.unpack_from('<I',elf,24)[0]!=0x400000 or not 0<len(binario)<=4032:raise ValueError('Entrada o tamano fuera del contrato NXE2')
    return struct.pack('<4sHHIIIIIII28s',b'NXE\0',2,1,64,0x400000,0,len(binario),8192,huella(binario),1,bytes(28))+binario
if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('elf',type=Path);p.add_argument('binario',type=Path);p.add_argument('salida',type=Path);a=p.parse_args()
    a.salida.write_bytes(empaquetar(a.elf.read_bytes(),a.binario.read_bytes()));print('NXE2 creado: '+str(a.salida))
