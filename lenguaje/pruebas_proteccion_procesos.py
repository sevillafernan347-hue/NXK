"""Proteccion privada real CPL3; usa imagen normal y discos nuevos con snapshot.

Contrato v0.0.26: INT81/25 cambia escritura0/1 de rangos de datos completos;
26 consulta una pagina. Los rechazos no aplican cambios parciales ni consumen
mensajes. Las pruebas de CPU y servicios usan aserciones dentro de cada NXE.
"""
import sys
import time
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parent))
from pruebas_dependencias import Codigo,Maquina,empaquetar,elf,MAXIMO
from pruebas_memoria_procesos import ceros,guardar,comprobar_memoria
from pruebas_buffers_procesos import datos,palabra,archivo,configurar
from pruebas_lotes_procesos import reservar_lote,liberar_lote
from pruebas_cuotas_procesos import compilar

RAIZ=Path(__file__).resolve().parents[1]
BASE=0x402000
ERROR=0xffffffff
RETENIDA=0xfffffffb
CRUCE=BASE+4092
BUFFER=BASE+8188


def proteger(c,p,n,escritura,esperado=0):
    c.imm('bb',p); c.imm('b9',n); c.imm('ba',escritura)
    c.servicio(25); c.comprobar('3d',esperado)


def consultar(c,p,esperado):
    c.imm('bb',p); c.servicio(26); c.comprobar('3d',esperado)


def actor_rangos():
    c=Codigo()
    consultar(c,BASE,ERROR)
    proteger(c,BASE,1,0,ERROR)
    reservar_lote(c,4,BASE)
    for i in range(4):
        consultar(c,BASE+i*4096,1); guardar(c,BASE+i*4096,100+i)
    for p,n,w in ((0,1,0),(0x400000,1,0),(0x401000,1,0),(BASE+1,1,0),
            (BASE,0,0),(BASE,5,0),(BASE,0xffffffff,0),(BASE,1,2),
            (BASE,1,0xffffffff),(BASE+12288,2,0),(0xfffff000,4,0)):
        proteger(c,p,n,w,ERROR)
    for i in range(4):consultar(c,BASE+i*4096,1)
    proteger(c,BASE,4,0)
    for i in range(4):consultar(c,BASE+i*4096,0)
    c.servicio(9); c.comprobar('3d',0)
    for i in range(4):
        consultar(c,BASE+i*4096,0); comprobar_memoria(c,BASE+i*4096,100+i)
    proteger(c,BASE+4096,2,1)
    for i in range(4):consultar(c,BASE+i*4096,int(i in (1,2)))
    guardar(c,BASE+4096,77); guardar(c,BASE+12284,88)
    liberar_lote(c,BASE+4096,1)
    # El segundo miembro es un hueco: el primer RO debe seguir RO.
    proteger(c,BASE,3,1,ERROR); consultar(c,BASE,0); consultar(c,BASE+8192,1)
    for p in (0,0x400000,0x401000,BASE+1,BASE+4096,BASE+16384,0xffffffff):
        consultar(c,p,ERROR)
    liberar_lote(c,BASE,1); liberar_lote(c,BASE+8192,2)
    reservar_lote(c,4,BASE)
    for i in range(4):consultar(c,BASE+i*4096,1); ceros(c,BASE+i*4096)
    c.salir(42); return c.terminar()


def actor_fallo(ceder=True):
    c=Codigo(); reservar_lote(c,1,BASE); guardar(c,BASE,77)
    proteger(c,BASE,1,0)
    if ceder:c.servicio(9); c.comprobar('3d',0)
    # Sin ceder prueba INVLPG sobre la entrada RW que uso el primer store.
    # Con otro despacho comprueba que reconstruir mapas conserva RO.
    guardar(c,BASE,99); c.salir(99); return c.terminar()


def actor_aislado():
    c=Codigo(); c.op('83 ec 08 89 e3'); c.recibir(); c.op('8b 73 04')
    reservar_lote(c,1,BASE); c.imm('89 35',BASE)
    c.imm('bb',BASE); c.imm('b9',1); c.op('89 f2'); c.servicio(25); c.comprobar('3d',0)
    c.servicio(9); c.comprobar('3d',0)
    c.imm('bb',BASE); c.servicio(26); c.op('39 f0'); c.salto('0f 85')
    c.imm('39 35',BASE); c.salto('0f 85')
    proteger(c,BASE,1,1); guardar(c,BASE,42); comprobar_memoria(c,BASE,42)
    c.salir(42); return c.terminar()


def actor_servicios():
    c=Codigo(); c.op('83 ec 08 89 e3'); c.recibir(); c.op('8b 6b 04')
    reservar_lote(c,2,BASE)
    nombre,fuente=BASE+128,BASE+4196
    datos(c,nombre,b'protegido\0'); datos(c,fuente,b'RO-DATOS')
    datos(c,CRUCE,bytes.fromhex('44 33 22 11 88 77 66 55'))
    proteger(c,BASE+4096,1,0)
    c.imm('bb',fuente); c.imm('b9',8); c.servicio(1); c.comprobar('3d',8)
    archivo(c,2,nombre,fuente,8)          # Guardar lee RO, por tanto permitido.
    for servicio in (3,5):
        archivo(c,servicio,nombre,CRUCE,8,ERROR)
        palabra(c,CRUCE,0x11223344); palabra(c,CRUCE+4,0x55667788)
    # Ni el mensaje pendiente ni el resultado se consumen con destino RO.
    c.imm('bb',CRUCE); c.imm('ba',0); c.servicio(12); c.comprobar('3d',ERROR)
    palabra(c,CRUCE,0x11223344); palabra(c,CRUCE+4,0x55667788)
    c.op('89 eb'); c.imm('b9',CRUCE); c.imm('ba',0)
    c.servicio(15); c.comprobar('3d',ERROR)
    palabra(c,CRUCE,0x11223344); palabra(c,CRUCE+4,0x55667788)
    proteger(c,BASE+4096,1,1)
    c.imm('bb',CRUCE); c.imm('ba',0); c.servicio(12); c.comprobar('3d',0)
    palabra(c,CRUCE,0); palabra(c,CRUCE+4,77)
    c.servicio(12); c.comprobar('3d',1)
    c.op('89 eb'); c.imm('b9',CRUCE); c.imm('ba',0)
    c.servicio(15); c.comprobar('3d',0)
    palabra(c,CRUCE,3); palabra(c,CRUCE+4,42)
    archivo(c,3,nombre,CRUCE,8,8)
    palabra(c,CRUCE,int.from_bytes(b'RO-D','little'))
    palabra(c,CRUCE+4,int.from_bytes(b'ATOS','little'))
    proteger(c,BASE,2,0); liberar_lote(c,BASE,2)
    reservar_lote(c,2,BASE); consultar(c,BASE,1); consultar(c,BASE+4096,1)
    ceros(c,BASE); ceros(c,BASE+4096)
    c.salir(42); return c.terminar()


def actor_espera(fin=False):
    c=Codigo(); c.op('83 ec 10 89 e3')
    # Recepcion: ticks/status/valor. FIN: destino/ticks/status/codigo.
    instrucciones=('8b 73 04','8b 7b 04','8b 6b 04','8b 43 04 89 44 24 0c') if fin else (
        '8b 7b 04','8b 6b 04','8b 43 04 89 44 24 0c')
    for instruccion in instrucciones:c.recibir(); c.op(instruccion)
    reservar_lote(c,3,BASE)
    datos(c,BUFFER,bytes.fromhex('44 33 22 11 88 77 66 55'))
    c.servicio(9); c.comprobar('3d',0)
    if fin:c.op('89 f3'); c.imm('b9',BUFFER)
    else:c.imm('bb',BUFFER)
    c.op('89 fa'); c.servicio(15 if fin else 12)
    c.op('39 e8'); c.salto('0f 85')
    c.op('85 c0'); c.salto('0f 85','vacio')
    palabra(c,BUFFER,3 if fin else 0)
    c.op('8b 44 24 0c'); c.imm('39 05',BUFFER+4); c.salto('0f 85')
    c.salto('e9','fin')
    c.etiquetas['vacio']=len(c.bytes)
    palabra(c,BUFFER,0x11223344); palabra(c,BUFFER+4,0x55667788)
    c.etiquetas['fin']=len(c.bytes)
    # Si proteger el rango fallo, tampoco cambio la primera pagina no retenida.
    for i in range(3):consultar(c,BASE+i*4096,1); guardar(c,BASE+i*4096,42+i)
    liberar_lote(c,BASE,3); c.salir(42); return c.terminar()


def actor_antiguo():
    c=Codigo()
    for n in (25,26):c.servicio(n); c.comprobar('3d',0xfffffffe)
    c.salir(0); return c.terminar()


def consola(m,id,p=BASE,n=3,w=0,exito=True):
    return m.comando(f'proceso proteger {id} {p} {n} {w}',
        b'Proteccion actualizada.' if exito else b'No se pudo cambiar proteccion.')


def probar_basicos(carpeta):
    archivos={'rangos.nxe':actor_rangos(),'fallo.nxe':actor_fallo(),
        'inmediato.nxe':actor_fallo(False),
        'aislar.nxe':actor_aislado(),'servicios.nxe':actor_servicios(),
        'antiguo.nxe':actor_antiguo(),'fin.nxe':empaquetar(elf,bytes.fromhex('b8 2a 00 00 00 cd 80')),
        'nextry.nxe':compilar(carpeta,'nextry',RAIZ/'lenguaje/ejemplos/proteccion_memoria.nxf')}
    m=Maquina(carpeta,'proteccion-basicos',archivos)
    try:
        m.esperar(0); m.comando('volumen montar',b'Volumen NXFS montado')
        m.comando('ejecutar usuario antiguo.nxe',b'Aplicacion NXE2 termino; consola recuperada.')
        m.comando('usuario estado',b'normal, salida 0')
        id=m.cargar('fin.nxe'); m.paso(id,3,42); m.liberar(id)
        m.comando('escribir calentar x'); m.comando('borrar calentar')
        memoria=m.comando('mem')
        for _ in range(2):
            id=m.cargar('rangos.nxe'); m.paso(id,1); m.paso(id,3,42); m.liberar(id)
            assert m.comando('mem')==memoria
        destino=m.cargar('fin.nxe'); m.paso(destino,3,42)
        id=m.cargar('servicios.nxe'); m.enviar(id,destino); m.enviar(id,77)
        assert b'RO-DATOS' in m.paso(id,3,42)
        assert m.info(id)['mensajes']==0
        m.comando('leer protegido',b'RO-DATOS'); m.comando('borrar protegido')
        m.liberar(id); m.liberar(destino); assert m.comando('mem')==memoria
        ids=[m.cargar('aislar.nxe') for _ in range(2)]
        for id,w in zip(ids,(0,1)):m.enviar(id,w); m.paso(id,1)
        fallo=m.cargar('fallo.nxe'); m.paso(fallo,1)
        r=m.paso(fallo,4)
        assert b'vector 14' in r and b'error 7' in r and b'direccion 0x00402000' in r,r
        inmediato=m.cargar('inmediato.nxe'); r=m.paso(inmediato,4)
        assert b'vector 14' in r and b'error 7' in r and b'direccion 0x00402000' in r,r
        for id in ids:m.paso(id,3,42); m.liberar(id)
        m.liberar(fallo); m.liberar(inmediato); assert m.comando('mem')==memoria
        id=m.cargar('nextry.nxe'); m.paso(id,1); m.paso(id,3,42); m.liberar(id)
        assert m.comando('mem')==memoria
        m.comando('ejecutar usuario nextry.nxe',b'No se pudo cargar o ejecutar NXE2.')
        m.comando('usuario estado',b'normal, salida 1')
        assert m.comando('mem')==memoria
    finally:m.cerrar()


def probar_retenciones(carpeta):
    archivos={'recibir.nxe':actor_espera(),'observar.nxe':actor_espera(True),
        'fin.nxe':empaquetar(elf,bytes.fromhex('b8 2a 00 00 00 cd 80'))}
    m=Maquina(carpeta,'proteccion-retenciones',archivos)
    try:
        m.esperar(0); m.comando('volumen montar',b'Volumen NXFS montado')
        id=m.cargar('fin.nxe'); m.paso(id,3,42); m.liberar(id)
        memoria=m.comando('mem')
        for fin in (False,True):
            for plazo in (False,True):
                destino=m.cargar('fin.nxe') if fin else None
                ticks=25 if plazo else MAXIMO
                valores=(destino,ticks,int(plazo),42) if fin else (ticks,int(plazo),42)
                id=configurar(m,'observar.nxe' if fin else 'recibir.nxe',valores)
                # Antes de prestar el buffer, el operador puede aplicar y retirar RO.
                consola(m,id); consola(m,id,w=1)
                m.paso(id,9 if fin else 6)
                consola(m,id,exito=False)     # Buffer empieza en la segunda pagina.
                consola(m,id,p=BASE+8192,n=1,exito=False)
                consola(m,id,w=1)           # Conservar RW no rompe la espera.
                consola(m,id,n=1); consola(m,id,n=1,w=1)
                if plazo:
                    time.sleep(.65)
                    assert m.info(id)['estado']==1
                    consola(m,id,exito=False)  # LISTO conserva buffer hasta despacho.
                elif fin:m.paso(destino,3,42)
                else:m.enviar(id,42)
                m.paso(id,3,42)
                m.liberar(id)
                if destino:m.liberar(destino)
                assert m.comando('mem')==memoria
            destino=m.cargar('fin.nxe') if fin else None
            valores=(destino,MAXIMO,0,42) if fin else (MAXIMO,0,42)
            id=configurar(m,'observar.nxe' if fin else 'recibir.nxe',valores)
            # Rechazos de sintaxis/rango no convierten un buffer RW en RO.
            for cola in ('',' 1',' -1 1 0',' 4294967296 1 0',f' {BASE} 0 0',
                    f' {BASE} 5 0',f' {BASE+1} 1 0',f' {BASE} 1 2',
                    f' {BASE} 1 0 extra'):
                m.comando(f'proceso proteger {id}'+cola,b'No se pudo cambiar proteccion.')
            m.paso(id,9 if fin else 6); consola(m,id,exito=False)
            m.cancelar(id); consola(m,id)      # Cancelar retira la retencion.
            viejo=id; m.liberar(id)
            consola(m,viejo,exito=False)
            if destino:m.liberar(destino)
            assert m.comando('mem')==memoria
        m.comando('proceso listar',b'Sin procesos registrados.')
    finally:m.cerrar()


def main():
    carpeta=RAIZ/'build/x86/proteccion-pruebas'; carpeta.mkdir(parents=True,exist_ok=True)
    probar_basicos(carpeta); probar_retenciones(carpeta)
    print('PROTECCION OK: RO/RW real, rangos atomicos, FS/IPC, retenciones, '
          'consola, aislamiento, NEXTRY, recursos recuperados y discos intactos.')


if __name__=='__main__':main()
