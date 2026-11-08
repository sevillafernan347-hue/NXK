"""Memoria dinamica privada CPL3, propuesta v0.0.20 sobre a0cf95e.

Contrato aprobado por integracion: INT81/18 reserva una pagina cero entre
402000 y405000, hasta cuatro por espacio, o devuelve0 sin reservar. INT81/19
libera la VA propia alineada; devuelve0 oFFFFFFFF, sin tocar otros registros.
La ruta no gestionada devuelveFFFFFFFE. Se usan imagenes nuevas y snapshots;
el kernel debe estar construido en build/x86/nxk.img antes de ejecutar.
"""
import os
import pathlib
import re
import subprocess
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))
from pruebas_dependencias import Codigo, Maquina

RAIZ = pathlib.Path(__file__).resolve().parents[1]
BASE = 0x402000
FIN = BASE + 4 * 4096
FUENTE_LIMITES = '''# Fuente generada para comprobar validacion del runtime.
funcion inicio
    entero operacion = 99
    entero direccion = 0
    entero origen = 0
    entero resultado = 99
    recibir_mensaje operacion, origen, resultado, 0
    afirmar resultado == 0
    recibir_mensaje direccion, origen, resultado, 0
    afirmar resultado == 0
    si operacion == 0
        numero leer_entero(direccion)
    fin
    si operacion == 1
        escribir_entero direccion, 42
    fin
    finalizar 99
fin
'''


def reservar(c, direccion):
    c.servicio(18)
    c.comprobar('3d', direccion)


def liberar(c, direccion, esperado=0):
    c.imm('bb', direccion)
    c.servicio(19)
    c.comprobar('3d', esperado)


def ceros(c, direccion):
    # REP SCASD examina toda la pagina, incluidos su primer y ultimo entero.
    c.imm('bf', direccion)
    c.imm('b9', 1024)
    c.op('31 c0 fc f3 af')
    c.salto('0f 85')


def guardar(c, direccion, valor):
    c.imm('c7 05', direccion)
    c.op(valor.to_bytes(4, 'little').hex())


def comprobar_memoria(c, direccion, valor):
    c.imm('81 3d', direccion)
    c.op(valor.to_bytes(4, 'little').hex())
    c.salto('0f 85')


def actor_limites():
    c = Codigo()
    # El cupo de memoria es independiente de permisos de archivos/salida/IPC.
    c.imm('bb', 0)
    c.servicio(16)
    c.comprobar('3d', 0)
    # CF/DF y EBX/ECX/EDX/ESI/EDI/EBP sobreviven a reserva y liberacion.
    for instruccion, valor in (('bb', 0x12345678), ('b9', 0x23456789),
                              ('ba', 0x3456789a), ('be', 0x456789ab),
                              ('bf', 0x56789abc), ('bd', 0x6789abcd)):
        c.imm(instruccion, valor)
    c.op('f9 fd')
    c.servicio(18)
    c.op('50 9c 58 fc')
    c.imm('25', 0x401)
    c.comprobar('3d', 0x401)
    c.op('58')
    c.comprobar('3d', BASE)
    for instruccion, valor in (('81 fb', 0x12345678), ('81 f9', 0x23456789),
                              ('81 fa', 0x3456789a), ('81 fe', 0x456789ab),
                              ('81 ff', 0x56789abc), ('81 fd', 0x6789abcd)):
        c.comprobar(instruccion, valor)
    c.comprobar('81 fc', 0x402000)
    c.imm('bb', BASE)
    c.op('f9 fd')
    c.servicio(19)
    c.op('50 9c 58 fc')
    c.imm('25', 0x401)
    c.comprobar('3d', 0x401)
    c.op('58')
    c.comprobar('3d', 0)
    for instruccion, valor in (('81 fb', BASE), ('81 f9', 0x23456789),
                              ('81 fa', 0x3456789a), ('81 fe', 0x456789ab),
                              ('81 ff', 0x56789abc), ('81 fd', 0x6789abcd)):
        c.comprobar(instruccion, valor)
    reservar(c, BASE)
    for i in range(4):
        pagina = BASE + i * 4096
        if i:
            reservar(c, pagina)
        ceros(c, pagina)
        guardar(c, pagina, 0x11000000 + i)
        guardar(c, pagina + 4092, 0x22000000 + i)
    reservar(c, 0)                       # El quinto intento no altera reservas.
    c.servicio(9)
    c.comprobar('3d', 0)
    for i in range(4):
        pagina = BASE + i * 4096
        comprobar_memoria(c, pagina, 0x11000000 + i)
        comprobar_memoria(c, pagina + 4092, 0x22000000 + i)
    for direccion in (0, 0x400000, 0x401000, BASE + 1, FIN, 0xffffffff):
        liberar(c, direccion, 0xffffffff)
    # Libera un hueco interior; los vecinos siguen intactos y se reutiliza
    # precisamente el hueco, con todos sus bytes puestos en cero.
    liberar(c, BASE + 4096)
    liberar(c, BASE + 4096, 0xffffffff)
    comprobar_memoria(c, BASE, 0x11000000)
    comprobar_memoria(c, BASE + 8192, 0x11000002)
    reservar(c, BASE + 4096)
    ceros(c, BASE + 4096)
    for i in range(3, -1, -1):
        liberar(c, BASE + i * 4096)
    for i in range(4):
        reservar(c, BASE + i * 4096)
        ceros(c, BASE + i * 4096)
    for i in range(4):
        liberar(c, BASE + i * 4096)
    c.salir(42)
    return c.terminar()


def actor_aislamiento():
    c = Codigo()
    c.op('83 ec 08 89 e3')
    c.recibir()
    c.op('8b 73 04')                      # Valor privado recibido del kernel.
    reservar(c, BASE)
    ceros(c, BASE)
    c.imm('89 35', BASE)
    c.imm('89 35', BASE + 4092)
    c.servicio(9)
    c.comprobar('3d', 0)
    for direccion in (BASE, BASE + 4092):
        c.imm('39 35', direccion)
        c.salto('0f 85')
    c.salir(42)                          # La pagina vive hasta liberar el PCB.
    return c.terminar()


def actor_ocupado():
    c = Codigo()
    for i in range(4):
        pagina = BASE + i * 4096
        reservar(c, pagina)
        ceros(c, pagina)
        guardar(c, pagina, 0xaabbcc00 + i)
        guardar(c, pagina + 4092, 0x11223300 + i)
    c.etiquetas['bucle'] = len(c.bytes)
    for i in range(4):
        comprobar_memoria(c, BASE + i * 4096, 0xaabbcc00 + i)
        comprobar_memoria(c, BASE + i * 4096 + 4092, 0x11223300 + i)
    c.salto('e9', 'bucle')               # PIT debe devolver la consola.
    return c.terminar()


def actor_liberado():
    c = Codigo()
    reservar(c, BASE)
    guardar(c, BASE, 0xdeadbeef)
    liberar(c, BASE)
    c.imm('a1', BASE)                    # Acceso posterior: fallo contenido14.
    c.salir(99)
    return c.terminar()


def actor_ausente():
    c = Codigo()
    c.imm('a1', BASE)                    # La pagina nunca fue reservada.
    c.salir(99)
    return c.terminar()


def actor_antiguo():
    c = Codigo()
    for numero in (18, 19):
        c.imm('bb', BASE)
        c.servicio(numero)
        c.comprobar('3d', 0xfffffffe)
    c.salir(0)
    return c.terminar()


def paginas_libres(m):
    datos = m.comando('mem')
    coincidencia = re.search(rb'Paginas fisicas libres/total \(4 KiB\): (\d+) / (\d+)', datos)
    assert coincidencia is not None, datos
    return int(coincidencia[1])


def main():
    carpeta = RAIZ / 'build/x86/memoria-procesos-pruebas'
    carpeta.mkdir(parents=True, exist_ok=True)
    entorno = {k: v for k, v in os.environ.items() if k.lower() != 'psmodulepath'}
    subprocess.run(['powershell.exe', '-NoProfile', '-ExecutionPolicy', 'Bypass', '-File',
                    str(RAIZ / 'compilar-usuario.ps1'), '-Fuente',
                    str(RAIZ/'lenguaje/ejemplos/pagina_memoria.nxf'), '-Salida',
                    str(carpeta / 'nextry.nxe')], cwd=RAIZ, env=entorno, check=True,
                   capture_output=True)
    # Este archivo es una entrada de prueba generada dentro de build, no fuente
    # del proyecto. Una app parametrizada cubre lectura y escritura negativas.
    (carpeta/'limites_runtime.nxf').write_text(FUENTE_LIMITES, encoding='utf-8')
    subprocess.run(['powershell.exe', '-NoProfile', '-ExecutionPolicy', 'Bypass', '-File',
                    str(RAIZ / 'compilar-usuario.ps1'), '-Fuente',
                    str(carpeta/'limites_runtime.nxf'), '-Salida',
                    str(carpeta/'runtime.nxe')], cwd=RAIZ, env=entorno, check=True,
                   capture_output=True)
    archivos = {'limites.nxe': actor_limites(), 'aislar.nxe': actor_aislamiento(),
                'ocupado.nxe': actor_ocupado(), 'liberado.nxe': actor_liberado(),
                'ausente.nxe': actor_ausente(), 'antiguo.nxe': actor_antiguo(),
                'nextry.nxe': (carpeta/'nextry.nxe').read_bytes(),
                'runtime.nxe': (carpeta/'runtime.nxe').read_bytes()}
    m = Maquina(carpeta, 'memoria-procesos', archivos)
    try:
        m.esperar(0)
        version = re.search(rb'#define NXK_VERSION "([^"]+)"', (RAIZ/'lib/version.h').read_bytes())[1]
        m.comando('version', version)
        m.comando('volumen montar', b'Volumen NXFS montado')
        # Calentar rutas de carga, ejecucion y liberacion antes de medir heap.
        id = m.cargar('ausente.nxe')
        assert b'fallo, vector 14' in m.paso(id, 4)
        m.liberar(id)
        m.comando('ejecutar usuario antiguo.nxe', b'Aplicacion NXE2 termino; consola recuperada.')
        m.comando('usuario estado', b'normal, salida 0')
        m.comando('ejecutar usuario nextry.nxe', b'No se pudo cargar o ejecutar NXE2.')
        m.comando('usuario estado', b'normal, salida 1')  # Reserva no soportada.
        memoria = m.comando('mem')
        libres = paginas_libres(m)

        for _ in range(2):
            id = m.cargar('limites.nxe')
            assert paginas_libres(m) == libres - 5
            m.paso(id, 1)
            assert paginas_libres(m) == libres - 9
            m.paso(id, 3, 42)
            assert paginas_libres(m) == libres - 5
            m.liberar(id)
            assert m.comando('mem') == memoria

        # Las mismas direcciones virtuales conservan contenidos distintos.
        a = m.cargar('aislar.nxe')
        b = m.cargar('aislar.nxe')
        for id, valor in ((a, 101), (b, 202)):
            m.enviar(id, valor)
            m.paso(id, 1)
        assert paginas_libres(m) == libres - 12
        for id in (a, b):
            m.paso(id, 3, 42)
        assert paginas_libres(m) == libres - 12  # Terminar retiene el diagnostico.
        for id in (a, b):
            m.liberar(id)
        assert m.comando('mem') == memoria

        # La preempcion repetida conserva las cuatro paginas sin nuevas reservas.
        id = m.cargar('ocupado.nxe')
        for n in range(1, 4):
            m.paso(id, 1)
            assert m.info(id)['preempciones'] == n
            assert paginas_libres(m) == libres - 9
        m.cancelar(id)
        assert paginas_libres(m) == libres - 9
        m.liberar(id)
        assert m.comando('mem') == memoria

        # Fallar o liberar mediante la ruta antigua tambien devuelve la pagina.
        for nombre in ('liberado.nxe', 'ausente.nxe'):
            id = m.cargar(nombre)
            datos = m.paso(id, 4)
            assert b'fallo, vector 14' in datos, datos
            m.liberar(id)
            assert m.comando('mem') == memoria
        id = m.cargar('aislar.nxe')
        m.enviar(id, 0xdeadbeef)
        m.paso(id, 1)
        m.liberar(id, antigua=True)
        assert m.comando('mem') == memoria
        viejo = id
        id = m.cargar('aislar.nxe')
        assert id != viejo
        m.liberar(viejo, False)
        m.enviar(id, 99)
        m.paso(id, 1)                   # Comprueba toda la pagina en cero.
        m.paso(id, 3, 42)
        m.liberar(id)
        assert m.comando('mem') == memoria
        for operacion in (0, 1):
            for direccion in (0, 0x400000, BASE + 1, FIN, 0xffffffff):
                id = m.cargar('runtime.nxe')
                m.enviar(id, operacion)
                m.enviar(id, direccion)
                m.paso(id, 3, 1)         # Validacion anterior al acceso CPU.
                m.liberar(id)
            id = m.cargar('runtime.nxe')
            m.enviar(id, operacion)
            m.enviar(id, BASE)
            assert b'fallo, vector 14' in m.paso(id, 4)
            m.liberar(id)
        assert m.comando('mem') == memoria

        id = m.cargar('nextry.nxe')
        m.paso(id, 1)
        m.paso(id, 3, 42)
        m.liberar(id)
        assert m.comando('mem') == memoria
        m.comando('proceso listar', b'Sin procesos registrados.')
        historial = m.comando('usuario historial')
        m.comando('proceso resumen')
        assert m.comando('usuario historial') == historial
        assert m.comando('mem') == memoria
    finally:
        m.cerrar()
    print('MEMORIA PROCESOS OK: cupo4, cero completo, limites, aislamiento, '
          'cesion/preempcion, fallos contenidos, liberacion, IDs, NEXTRY y memoria estable.')


if __name__ == '__main__':
    main()
