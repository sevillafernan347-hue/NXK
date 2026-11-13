"""Lotes privados reales: todo o nada, fragmentacion, cuota e aislamiento."""
import re
import sys
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parent))
from pruebas_dependencias import Codigo,Maquina
from pruebas_memoria_procesos import ceros,guardar,comprobar_memoria,liberar
from pruebas_cuotas_procesos import compilar,estado,limitar,RAIZ,BASE

def reservar_lote(c,n,esperado):
    c.imm('bb',n); c.servicio(23); c.comprobar('3d',esperado)

def liberar_lote(c,p,n,esperado=0):
    c.imm('bb',p); c.imm('b9',n); c.servicio(24); c.comprobar('3d',esperado)

def comprobar_paginas(c,n=4):
    for i in range(n):
        p=BASE+i*4096; ceros(c,p); guardar(c,p,100+i); guardar(c,p+4092,200+i)

def actor():
    c=Codigo()
    for n in (0,5,0xffffffff):reservar_lote(c,n,0)
    reservar_lote(c,4,BASE); comprobar_paginas(c)
    c.servicio(9); c.comprobar('3d',0)
    for i in range(4):comprobar_memoria(c,BASE+i*4096,100+i)
    liberar_lote(c,BASE+4096,2)
    liberar_lote(c,BASE,4,0xffffffff)      # Hueco: ni la primera se libera.
    comprobar_memoria(c,BASE,100); comprobar_memoria(c,BASE+12288,103)
    reservar_lote(c,3,0)                 # Cuota insuficiente.
    reservar_lote(c,2,BASE+4096)
    ceros(c,BASE+4096); ceros(c,BASE+8192)
    liberar(c,BASE); liberar(c,BASE+12288)
    reservar_lote(c,2,0)                 # Hay dos huecos, ninguno de dos paginas.
    c.servicio(18); c.comprobar('3d',BASE)
    liberar_lote(c,BASE,3)               # Mezcla reservas individuales y lote.
    for p in (0,0x400000,0x401000,BASE+1,BASE+16384,0xffffffff):
        liberar_lote(c,p,1,0xffffffff)
    for n in (0,5,0xffffffff):liberar_lote(c,BASE,n,0xffffffff)
    reservar_lote(c,4,BASE); comprobar_paginas(c)
    liberar_lote(c,BASE+12288,2,0xffffffff)
    comprobar_memoria(c,BASE+12288,103)
    c.salir(42)                         # Destructor recupera el lote no liberado.
    return c.terminar()

def actor_cuota():
    c=Codigo(); limitar(c,2)
    reservar_lote(c,3,0); reservar_lote(c,2,BASE); reservar_lote(c,1,0)
    liberar_lote(c,BASE,2); limitar(c,0); reservar_lote(c,1,0)
    c.salir(42); return c.terminar()

def aislado():
    c=Codigo(); c.op('83 ec 08 89 e3'); c.recibir(); c.op('8b 73 04')
    reservar_lote(c,2,BASE)
    for p in (BASE,BASE+8188):c.imm('89 35',p)
    c.servicio(9); c.comprobar('3d',0)
    for p in (BASE,BASE+8188):
        c.imm('39 35',p); c.salto('0f 85')
    liberar_lote(c,BASE,2); c.salir(42); return c.terminar()

def antiguo():
    c=Codigo()
    for n in (23,24):c.servicio(n); c.comprobar('3d',0xfffffffe)
    c.salir(0); return c.terminar()

def main():
    carpeta=RAIZ/'build/x86/lotes-pruebas'; carpeta.mkdir(parents=True,exist_ok=True)
    archivos={'lotes.nxe':actor(),'cuota.nxe':actor_cuota(),'aislar.nxe':aislado(),
        'antiguo.nxe':antiguo(),
        'nextry.nxe':compilar(carpeta,'nextry',RAIZ/'lenguaje/ejemplos/paginas_lote.nxf')}
    m=Maquina(carpeta,'lotes',archivos)
    try:
        m.esperar(0)
        version=re.search(rb'#define NXK_VERSION "([^"]+)"',(RAIZ/'lib/version.h').read_bytes())[1]
        m.comando('version',version); m.comando('volumen montar',b'Volumen NXFS montado')
        m.comando('ejecutar usuario antiguo.nxe',b'Aplicacion NXE2 termino; consola recuperada.')
        id=m.cargar('lotes.nxe'); m.paso(id,1); m.paso(id,3,42); m.liberar(id)
        memoria=m.comando('mem')
        for _ in range(2):
            id=m.cargar('lotes.nxe'); m.paso(id,1)
            assert estado(m,id)==(4,4)
            m.paso(id,3,42); assert estado(m,id)==(4,4); m.liberar(id)
            id=m.cargar('cuota.nxe'); m.paso(id,3,42)
            assert estado(m,id)==(0,0); m.liberar(id)
            assert m.comando('mem')==memoria
        ids=[m.cargar('aislar.nxe') for _ in range(2)]
        for id,valor in zip(ids,(101,202)):m.enviar(id,valor); m.paso(id,1)
        for id in ids:m.paso(id,3,42); m.liberar(id)
        assert m.comando('mem')==memoria
        id=m.cargar('lotes.nxe'); m.paso(id,1); m.cancelar(id); m.liberar(id)
        assert m.comando('mem')==memoria
        id=m.cargar('nextry.nxe'); m.paso(id,1); m.paso(id,3,42); m.liberar(id)
        assert m.comando('mem')==memoria
        m.comando('ejecutar usuario nextry.nxe',b'No se pudo cargar o ejecutar NXE2.')
        m.comando('usuario estado',b'normal, salida 1')
        assert m.comando('mem')==memoria
    finally:m.cerrar()
    print('LOTES OK: rangos atomicos, fragmentacion, cuota, aislamiento, NEXTRY y memoria estable.')

if __name__=='__main__':main()
