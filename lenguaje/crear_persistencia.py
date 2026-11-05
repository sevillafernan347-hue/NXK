"""Crea un disco virtual NXPD1 nuevo; nunca reemplaza un archivo existente."""
import argparse, struct
from pathlib import Path

def huella(datos):
    h=2166136261
    for b in datos:h=((h^b)*16777619)&0xffffffff
    return h

def crear():
    disco=bytearray(19*512)
    disco[:8]=b'NXPD0001'
    struct.pack_into('<IIII',disco,8,1,19,8,512)
    struct.pack_into('<I',disco,508,huella(disco[:508]))
    return disco

if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__); p.add_argument('salida',type=Path)
    a=p.parse_args()
    if a.salida.suffix.lower()!='.img':p.error('La salida debe ser un archivo .img nuevo.')
    with a.salida.open('xb') as f:f.write(crear())
    print('Disco NXPD1 creado: '+str(a.salida))
