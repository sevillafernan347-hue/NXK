"""Cuotas reales CPL3: aislamiento, limites monotones y recursos recuperados."""
import os
import re
import sys
import subprocess
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parent))
from pruebas_dependencias import Codigo,Maquina
from pruebas_memoria_procesos import reservar,liberar,guardar,comprobar_memoria
from pruebas_buffers_procesos import actor_recibir,configurar
RAIZ=Path(__file__).resolve().parents[1]
BASE=0x402000

def cuota(c,n):
    c.servicio(22); c.comprobar('3d',n)

def limitar(c,n,resultado=0):
    c.imm('bb',n); c.servicio(21); c.comprobar('3d',resultado)

def actor():
    c=Codigo()
    cuota(c,4)
    for n in (5,0xffffffff): limitar(c,n,0xffffffff)
    cuota(c,4)
    # Los permisos de archivos/salida/IPC son independientes de memoria.
    c.imm('bb',0); c.servicio(16); c.comprobar('3d',0)
    limitar(c,2); limitar(c,2)
    reservar(c,BASE); reservar(c,BASE+4096); reservar(c,0)
    guardar(c,BASE,123)
    limitar(c,1,0xfffffffe)             # Uso actual superior; no retira datos.
    cuota(c,2); comprobar_memoria(c,BASE,123)
    c.servicio(9); c.comprobar('3d',0)
    cuota(c,2); comprobar_memoria(c,BASE,123)
    liberar(c,BASE+4096); limitar(c,1)
    limitar(c,2,0xfffffffb); reservar(c,0)
    liberar(c,BASE); limitar(c,0)
    cuota(c,0); limitar(c,1,0xfffffffb); reservar(c,0)
    c.salir(42)
    return c.terminar()

def antiguo():
    c=Codigo()
    for n in (21,22):
        c.servicio(n); c.comprobar('3d',0xfffffffe)
    c.salir(0); return c.terminar()

def compilar(carpeta,nombre,fuente):
    entorno={k:v for k,v in os.environ.items() if k.lower()!='psmodulepath'}
    salida=carpeta/(nombre+'.nxe')
    subprocess.run(['powershell.exe','-NoProfile','-ExecutionPolicy','Bypass','-File',
        str(RAIZ/'compilar-usuario.ps1'),'-Fuente',str(fuente),'-Salida',str(salida)],
        cwd=RAIZ,env=entorno,check=True,capture_output=True)
    return salida.read_bytes()

def estado(m,id):
    d=m.comando('usuario listar')
    r=re.search(rb'Programa privado ID '+str(id).encode()+rb' paginas_datos (\d+) cuota_datos (\d+)',d)
    assert r is not None,d
    return tuple(map(int,r.groups()))

def reducir(m,id,n,exito=True):
    return m.comando(f'proceso memoria {id} {n}',b'Cuota reducida.' if exito else b'No se pudo reducir cuota.')

def main():
    carpeta=RAIZ/'build/x86/cuotas-pruebas'; carpeta.mkdir(parents=True,exist_ok=True)
    # Entrada generada solo para verificar cuotas diferentes en los cuatro slots.
    fuente=carpeta/'sonda.nxf'
    fuente.write_text('''funcion inicio
entero esperado = 0
entero origen = 0
entero resultado = 0
recibir_mensaje esperado, origen, resultado, 0
afirmar resultado == 0
afirmar cuota_memoria() == esperado
entero indice = 0
repetir 4 veces
entero pagina = reservar_pagina()
si indice < esperado
afirmar pagina != 0
escribir_entero pagina, indice + 100
sino
afirmar pagina == 0
fin
indice = indice + 1
fin
ceder
afirmar cuota_memoria() == esperado
finalizar 42
fin
''',encoding='utf-8')
    rechazo=carpeta/'rechazo.nxf'
    rechazo.write_text('funcion inicio\nlimitar_memoria 1\nentero pagina = reservar_pagina()\n'
        'afirmar pagina != 0\nlimitar_memoria 0\nfinalizar 99\nfin\n',encoding='utf-8')
    archivos={'cuota.nxe':actor(),'antiguo.nxe':antiguo(),'recibir.nxe':actor_recibir(),
        'rechazo.nxe':compilar(carpeta,'rechazo',rechazo),
        'sonda.nxe':compilar(carpeta,'sonda',fuente),
        'nextry.nxe':compilar(carpeta,'nextry',RAIZ/'lenguaje/ejemplos/cuota_memoria.nxf')}
    m=Maquina(carpeta,'cuotas',archivos)
    try:
        m.esperar(0)
        version=re.search(rb'#define NXK_VERSION "([^"]+)"',(RAIZ/'lib/version.h').read_bytes())[1]
        m.comando('version',version)
        m.comando('volumen montar',b'Volumen NXFS montado')
        m.comando('ejecutar usuario antiguo.nxe',b'Aplicacion NXE2 termino; consola recuperada.')
        # Primera ejecucion calienta la pila kernel reutilizable.
        id=m.cargar('cuota.nxe'); m.paso(id,1)
        assert estado(m,id)==(2,2)
        for n in (0,1,3,5,4294967296,-1):reducir(m,id,n,False)
        assert estado(m,id)==(2,2)
        reducir(m,id,2); m.paso(id,3,42)
        assert estado(m,id)==(0,0)
        m.liberar(id)
        memoria=m.comando('mem')
        ids=[]
        for n in range(4):
            id=m.cargar('sonda.nxe'); ids.append(id)
            assert estado(m,id)==(0,4)
            reducir(m,id,n); m.enviar(id,n)
        for n,id in enumerate(ids):
            m.paso(id,1); assert estado(m,id)==(n,n)
            reducir(m,id,n+1,False)
            if n:reducir(m,id,n-1,False)
        for n,id in enumerate(ids):
            m.cancelar(id); assert estado(m,id)==(n,n)
            m.liberar(id)
        assert m.comando('mem')==memoria
        viejo=ids[0]
        # Reducir no invalida un buffer de recepcion suspendida de dos paginas.
        receptor=configurar(m,'recibir.nxe',(0x7fffffff,0,42))
        reducir(m,receptor,2); m.paso(receptor,6)
        reducir(m,receptor,1,False)
        assert estado(m,receptor)==(2,2)
        m.enviar(receptor,42); m.paso(receptor,3,42)
        reducir(m,receptor,0)
        assert estado(m,receptor)==(0,0)
        m.liberar(receptor)
        assert m.comando('mem')==memoria
        id=m.cargar('nextry.nxe')
        assert id!=viejo and estado(m,id)==(0,4)
        reducir(m,viejo,0,False)
        m.paso(id,1); m.paso(id,3,42); m.liberar(id)
        assert m.comando('mem')==memoria
        m.comando('ejecutar usuario nextry.nxe',b'No se pudo cargar o ejecutar NXE2.')
        m.comando('usuario estado',b'normal, salida 1')
        assert m.comando('mem')==memoria
        id=m.cargar('rechazo.nxe'); m.paso(id,3,1)
        assert estado(m,id)==(1,1)       # Runtime falla sin retirar la pagina.
        m.liberar(id)
        assert m.comando('mem')==memoria
    finally:m.cerrar()
    print('CUOTAS OK: limites, uso conservado, cuatro slots, consola, IDs, NEXTRY y memoria estable.')

if __name__=='__main__':main()
