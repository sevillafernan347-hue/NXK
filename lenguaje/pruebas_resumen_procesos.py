"""Resumen de procesos: estados mixtos, referencias y consulta sin efectos.

Base397024b. Prueba el contrato de v0.0.18 mediante dos imagenes QEMU con
snapshot; no escribe discos fisicos. El resumen refleja estados almacenados:
no despierta procesos por plazo ni depura PCB cuyo espacio ya se libero.
Requiere construir antes la imagen x86 normal desde la integracion.
"""
import pathlib
import re
import sys
import time

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))
from pruebas_dependencias import Maquina, programas_raw, empaquetar, elf

RAIZ = pathlib.Path(__file__).resolve().parents[1]
NOMBRES = ('libre listo ejecutando terminado fallido dormido recibir '
           'cancelado enviar fin').split()


def resumen(maquina):
    datos = maquina.comando('proceso resumen')
    cabecera = re.search(
        rb'Procesos registrados (\d+) pendientes (\d+) finalizados (\d+) '
        rb'mensajes (\d+) referencias_fin (\d+)\r?\n', datos)
    estados = re.search(rb'Estados:' + b''.join(
        b' ' + nombre.encode() + rb' (\d+)' for nombre in NOMBRES), datos)
    assert cabecera is not None and estados is not None, datos
    r = dict(zip(('registrados', 'pendientes', 'finalizados', 'mensajes',
                  'referencias_fin'), map(int, cabecera.groups())))
    r['estados'] = tuple(map(int, estados.groups()))
    # Las categorias forman una particion de los cuatro slots disponibles.
    e = r['estados']
    assert sum(e) == 4 and 4 - e[0] == r['registrados'], r
    assert e[3] + e[4] + e[7] == r['finalizados'], r
    assert e[1] + e[2] + e[5] + e[6] + e[8] + e[9] == r['pendientes'], r
    return r


def comprobar(maquina, estados, mensajes=0, referencias=0):
    r = resumen(maquina)
    assert r['estados'] == tuple(estados), r
    assert r['mensajes'] == mensajes and r['referencias_fin'] == referencias, r
    return r


def main():
    carpeta = RAIZ / 'build/x86/resumen-pruebas'
    carpeta.mkdir(parents=True, exist_ok=True)
    archivos = programas_raw()
    # Mantener las ocho entradas de NXFS, sustituyendo un actor que no se usa.
    del archivos['limites.nxe']
    archivos['breve.nxe'] = empaquetar(elf, bytes.fromhex(
        'bb 1e 00 00 00 b8 0a 00 00 00 cd 81 b8 07 00 00 00 cd 80'))
    m = Maquina(carpeta, 'resumen-procesos', archivos)
    vacio = (4, 0, 0, 0, 0, 0, 0, 0, 0, 0)
    try:
        m.esperar(0)
        version = re.search(rb'#define NXK_VERSION "([^"]+)"', (RAIZ/'lib/version.h').read_bytes())[1]
        m.comando('version', version)
        m.comando('help', b'proceso resumen')
        comprobar(m, vacio)
        m.comando('volumen montar', b'Volumen NXFS montado')
        # Calentar el buffer de lectura y la pila fija del motor antes de mem.
        id = m.cargar('fin.nxe')
        m.paso(id, 3, 42)
        m.liberar(id)
        memoria = m.comando('mem')
        historial = m.comando('usuario historial')
        for _ in range(3):
            comprobar(m, vacio)
        assert m.comando('mem') == memoria
        assert m.comando('usuario historial') == historial

        # Vence el plazo real, pero una consulta de resumen no cambia DORMIDO.
        breve = m.cargar('breve.nxe')
        m.paso(breve, 5)
        historial = m.comando('usuario historial')
        ocupada = m.comando('mem')
        time.sleep(.6)
        for _ in range(3):
            comprobar(m, (3, 0, 0, 0, 0, 1, 0, 0, 0, 0))
        assert m.comando('mem') == ocupada
        assert m.comando('usuario historial') == historial
        # Listar si refresca el plazo segun el contrato anterior; resumen lo ve.
        assert m.info(breve)['estado'] == 1
        comprobar(m, (3, 1, 0, 0, 0, 0, 0, 0, 0, 0))
        m.paso(breve, 3, 7)
        comprobar(m, (3, 0, 0, 1, 0, 0, 0, 0, 0, 0))
        m.liberar(breve)
        comprobar(m, vacio)

        # Cuatro slots: DORMIDO, LISTO con un mensaje, FIN y CANCELADO.
        dormido = m.cargar('vivo.nxe')
        m.paso(dormido, 5)
        trabajador = m.cargar('fin.nxe')
        m.enviar(trabajador, 17)
        observador = m.configurar_observador(trabajador)
        m.paso(observador, 9)
        cancelado = m.cargar('fin.nxe')
        m.enviar(cancelado, 23)
        m.cancelar(cancelado)                 # Cancelar limpia su propia cola.
        antes = comprobar(m, (0, 1, 0, 0, 0, 1, 0, 1, 0, 1), 1, 1)
        detalle = m.estados()
        ocupada = m.comando('mem')
        historial = m.comando('usuario historial')
        for _ in range(4):
            assert resumen(m) == antes
        assert m.estados() == detalle
        assert m.comando('mem') == ocupada
        assert m.comando('usuario historial') == historial

        # Cancelar al observador retira la referencia, manteniendo ambos IDs.
        m.cancelar(observador)
        comprobar(m, (0, 1, 0, 0, 0, 1, 0, 2, 0, 0), 1, 0)
        # La API antigua deja un PCB obsoleto: resumen lo omite sin enumerarlo.
        m.liberar(dormido, antigua=True)
        antes = comprobar(m, (1, 1, 0, 0, 0, 0, 0, 2, 0, 0), 1, 0)
        assert resumen(m) == antes
        # Los mensajes de un proceso terminado siguen contandose hasta liberar.
        m.paso(trabajador, 3, 42)
        comprobar(m, (1, 0, 0, 1, 0, 0, 0, 2, 0, 0), 1, 0)
        m.liberar(trabajador)
        comprobar(m, (2, 0, 0, 0, 0, 0, 0, 2, 0, 0))
        m.liberar(observador)
        m.liberar(cancelado)
        comprobar(m, vacio)
        assert m.comando('mem') == memoria
        m.comando('proceso dependencias',
                  b'Dependencias: resultado42 recibido y liberacion anticipada protegida.')
        comprobar(m, vacio)
        assert m.comando('mem') == memoria
    finally:
        m.cerrar()
    print('RESUMEN PROCESOS OK: estados mixtos, referencias, cola, IDs obsoletos, '
          'consulta sin efectos y memoria estable.')


if __name__ == '__main__':
    main()
