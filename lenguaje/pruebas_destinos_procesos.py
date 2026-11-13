"""Filtros salientes reales: syscall, revocacion y generacion de procesos."""
import sys
import time
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parent))
from pruebas_dependencias import Codigo,Maquina,emisor,empaquetar,elf,MAXIMO
from pruebas_memoria_procesos import guardar
from pruebas_cuotas_procesos import compilar
RAIZ=Path(__file__).resolve().parents[1]
ERROR=0xffffffff
DENEGADO=0xfffffffb

def lista(c,p,n,r=0):
    c.imm('bb',p); c.imm('b9',n); c.servicio(27); c.comprobar('3d',r)

def politica():
    c=Codigo(); c.servicio(13); c.op('89 c6')
    c.imm('bb',2); c.servicio(23); c.comprobar('3d',0x402000)
    c.imm('89 35',0x402ffc); guardar(c,0x403000,0xffffff03)
    c.imm('bb',0x402000); c.imm('b9',2); c.imm('ba',0); c.servicio(25); c.comprobar('3d',0)
    # Tabla legible cruzada y RO: no requiere escritura en el proceso.
    lista(c,0x402ffc,2)
    for p,n in ((0,1),(0x403ffc,2),(0xffffffff,4),(0x402000,5)):
        lista(c,p,n,ERROR)
    c.imm('89 35',0x401f80); c.imm('89 35',0x401f84)
    lista(c,0x401f80,2,ERROR)
    for invalido in (0,1,0x100,0x105,0xffffffff):
        guardar(c,0x401f90,invalido); lista(c,0x401f90,1,ERROR)
    c.op('89 f3'); c.imm('b9',42); c.servicio(11); c.comprobar('3d',0)
    c.imm('bb',0x401fa0); c.imm('ba',0); c.recibir(); c.comprobar('81 7b 04',42)
    lista(c,0x401f80,1); lista(c,0x402ffc,2,DENEGADO)
    for servicio in (11,14):
        c.imm('bb',0xffffff03); c.imm('b9',77); c.imm('ba',0)
        c.servicio(servicio); c.comprobar('3d',DENEGADO)
    c.imm('bb',7); c.servicio(16); c.comprobar('3d',0)
    c.op('89 f3'); c.imm('b9',42); c.servicio(11); c.comprobar('3d',DENEGADO)
    lista(c,0xffffffff,0); lista(c,0x401f80,1,DENEGADO)
    c.salir(42); return c.terminar()

def antiguo():
    c=Codigo(); c.servicio(27); c.comprobar('3d',0xfffffffe); c.salir(0); return c.terminar()

def limitar(m,id,ids=(),exito=True):
    sufijo=' '+','.join(map(str,ids)) if ids else ''
    m.comando(f'proceso destinos {id}'+sufijo,
        b'Destinos restringidos.' if exito else b'No se pudo restringir destinos.')

def main():
    carpeta=RAIZ/'build/x86/destinos-pruebas'; carpeta.mkdir(parents=True,exist_ok=True)
    archivos={'politica.nxe':politica(),'enviar.nxe':emisor(),'antiguo.nxe':antiguo(),
        'fin.nxe':empaquetar(elf,bytes.fromhex('b8 2a 00 00 00 cd 80')),
        'nextry.nxe':compilar(carpeta,'nextry',RAIZ/'lenguaje/ejemplos/destinos_mensajes.nxf')}
    m=Maquina(carpeta,'destinos',archivos)
    try:
        m.esperar(0); m.comando('volumen montar',b'Volumen NXFS montado')
        m.comando('ejecutar usuario antiguo.nxe',b'Aplicacion NXE2 termino; consola recuperada.')
        id=m.cargar('politica.nxe'); m.paso(id,3,42); m.liberar(id)
        memoria=m.comando('mem')
        for plazo in (False,True):
            destino=m.cargar('fin.nxe'); m.llenar(destino)
            id=m.configurar_emisor(destino,25 if plazo else MAXIMO)
            limitar(m,id,[destino]); m.paso(id,8)
            if plazo:
                time.sleep(.5); assert m.info(id)['estado']==1
            limitar(m,id); m.paso(id,3,DENEGADO)
            assert m.info(id)['enviados']==0 and m.info(id)['ciclos']==0
            assert m.info(destino)['mensajes']==4
            limitar(m,id,(),False); m.liberar(id); m.cancelar(destino); m.liberar(destino)
            assert m.comando('mem')==memoria
        viejo=m.cargar('fin.nxe'); m.cancelar(viejo); m.liberar(viejo)
        nuevo=m.cargar('fin.nxe'); assert nuevo!=viejo and (nuevo&255)==(viejo&255)
        id=m.configurar_emisor(nuevo); limitar(m,id,[viejo])
        limitar(m,id,[nuevo],False); m.paso(id,3,DENEGADO)
        assert m.info(nuevo)['mensajes']==0
        m.liberar(id)
        # Nuevas generaciones no heredan el filtro; sintaxis rechazada no aplica.
        id=m.configurar_emisor(nuevo)
        for sufijo in (' 1',' 0',' -1',' 4294967296',' 257,',' ,257',' 257,257',' 257,258,259,260,513'):
            m.comando(f'proceso destinos {id}'+sufijo,b'No se pudo restringir destinos.')
        m.paso(id,3,0); assert m.info(nuevo)['mensajes']==1; m.liberar(id)
        id=m.cargar('nextry.nxe'); m.enviar(id,nuevo); m.paso(id,1)
        assert m.info(nuevo)['mensajes']==2
        m.paso(id,3,42); m.liberar(id); m.cancelar(nuevo); m.liberar(nuevo)
        assert m.comando('mem')==memoria
        m.comando('proceso listar',b'Sin procesos registrados.')
    finally:m.cerrar()
    print('DESTINOS OK: listas, tabla RO/cruzada, revocacion pendiente, IDs, consola, NEXTRY y recursos estables.')

if __name__=='__main__':main()
