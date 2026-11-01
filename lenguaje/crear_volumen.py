"""Crea una imagen virtual NXFS1 nueva; no abre discos fisicos ni sobrescribe archivos."""
import argparse,struct,re
from pathlib import Path
def huella(datos):
    h=2166136261
    for b in datos: h=((h^b)*16777619)&0xffffffff
    return h
def crear(archivos):
    if len(archivos)>8: raise ValueError('Maximo ocho archivos')
    imagen=bytearray(1024*1024); imagen[:4]=b'NXFS'
    struct.pack_into('<II',imagen,4,1,len(archivos)); lba=1
    for i,(nombre,datos) in enumerate(archivos.items()):
        if not re.fullmatch(r'[a-z0-9_.-]{1,23}',nombre) or len(datos)>4096: raise ValueError('Nombre o tamano invalido')
        pos=16+i*40; imagen[pos:pos+len(nombre)]=nombre.encode('ascii')
        struct.pack_into('<III',imagen,pos+24,len(datos),lba,huella(datos))
        imagen[lba*512:lba*512+len(datos)]=datos; lba+=max(1,(len(datos)+511)//512)
    struct.pack_into('<I',imagen,508,huella(imagen[:508])); return imagen
if __name__=='__main__':
    p=argparse.ArgumentParser(); p.add_argument('salida',type=Path); p.add_argument('--archivo',action='append',default=[]); a=p.parse_args()
    archivos={}
    for entrada in a.archivo:
        nombre,ruta=entrada.split('=',1)
        if nombre in archivos: raise ValueError('Nombre duplicado')
        archivos[nombre]=Path(ruta).read_bytes()
    if not a.archivo: archivos={'leeme.txt':b'NXFS: archivo leido desde disco virtual.\n'}
    imagen=crear(archivos)
    with a.salida.open('xb') as f: f.write(imagen)
    print(a.salida)
