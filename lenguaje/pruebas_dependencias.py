"""Dependencias CPL3 v0.0.17, base e755e00: envio con plazo y resultados.

Prueba los servicios14/15 mediante pequenos NXE y tambien desde NEXTRY.
Los dos volumenes NXFS son nuevos y QEMU siempre usa snapshot. No accede a
discos fisicos. Requiere imagen x86 normal ya construida por la integracion.
"""
import os
import pathlib
import re
import struct
import subprocess
import sys
import time

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))
from crear_volumen import crear
from empaquetar_usuario import empaquetar

RAIZ = pathlib.Path(__file__).resolve().parents[1]
MAXIMO = 0x7fffffff
elf = bytearray(52)
elf[:7] = b'\x7fELF\x01\x01\x01'
struct.pack_into('<HHII', elf, 16, 2, 3, 1, 0x400000)


class Codigo:
    """Emite instrucciones x86 fijas y relocaliza saltos de las aserciones."""
    def __init__(self):
        self.bytes = bytearray()
        self.saltos = []
        self.etiquetas = {}

    def op(self, texto):
        self.bytes.extend(bytes.fromhex(texto))

    def imm(self, op, valor):
        self.op(op)
        self.bytes.extend(struct.pack('<I', valor))

    def salto(self, op, destino='fallo'):
        self.op(op)
        self.saltos.append((len(self.bytes), destino))
        self.bytes.extend(bytes(4))

    def comprobar(self, op, valor):
        self.imm(op, valor)
        self.salto('0f 85')

    def servicio(self, numero):
        self.imm('b8', numero)
        self.op('cd 81')

    def recibir(self):
        self.imm('ba', 0)
        self.servicio(12)
        self.comprobar('3d', 0)

    def salir(self, valor):
        self.imm('b8', valor)
        self.op('fc cd 80')

    def terminar(self):
        self.etiquetas['fallo'] = len(self.bytes)
        self.op('fc 0f 0b')
        for posicion, destino in self.saltos:
            struct.pack_into('<i', self.bytes, posicion,
                             self.etiquetas[destino] - posicion - 4)
        return empaquetar(elf, self.bytes)


def reservar(c, cantidad=8):
    c.op('83 ec ' + f'{cantidad:02x}' + ' 89 e3')


def sellar(c):
    c.imm('c7 04 24', 0x11223344)
    c.imm('c7 44 24 04', 0x55667788)


def intacto(c):
    c.comprobar('81 3c 24', 0x11223344)
    c.comprobar('81 7c 24 04', 0x55667788)


def guardar_retorno_y_banderas(c):
    c.op('89 44 24 0c 9c 8f 44 24 08 fc 8b 44 24 08')
    c.imm('25', 0x401)
    c.comprobar('3d', 0x401)  # CF y DF se guardan antes de cualquier CMP.


def emisor():
    c = Codigo()
    reservar(c, 16)
    # Tres mensajes de configuracion: destino, valor y ticks. Ceder permite
    # llenar ambas colas antes del envio y construir ciclos reales de espera.
    for op in ('8b 73 04', '8b 7b 04', '8b 6b 04'):
        c.recibir()
        c.op(op)
    c.servicio(9)
    c.comprobar('3d', 0)
    c.op('89 f3 89 f9 89 ea f9 fd')
    c.servicio(14)
    guardar_retorno_y_banderas(c)
    for comparar in ('39 f3', '39 f9', '39 ea'):
        c.op(comparar)
        c.salto('0f 85')
    c.comprobar('81 fc', 0x401ff0)
    c.op('8b 44 24 0c fc cd 80')
    return c.terminar()


def observador():
    c = Codigo()
    reservar(c, 24)
    # Destino, ticks, estado esperado y codigo esperado. El resultado inicial
    # se sella despues de consumir la configuracion para comprobar timeouts.
    c.recibir()
    c.op('8b 73 04')
    c.recibir()
    c.op('8b 43 04 89 44 24 10')
    c.recibir()
    c.op('8b 7b 04')
    c.recibir()
    c.op('8b 6b 04')
    sellar(c)
    c.op('89 f3 89 e1 8b 54 24 10 f9 fd')
    c.servicio(15)
    guardar_retorno_y_banderas(c)
    for comparar in ('39 f3', '39 e1', '3b 54 24 10'):
        c.op(comparar)
        c.salto('0f 85')
    c.comprobar('81 fc', 0x401fe8)
    c.op('8b 44 24 0c 85 c0')
    c.salto('0f 84', 'resultado')
    intacto(c)
    c.op('fc cd 80')                       # 1 o error: ocho bytes intactos.
    c.etiquetas['resultado'] = len(c.bytes)
    c.op('39 3c 24')
    c.salto('0f 85')
    c.op('39 6c 24 04')
    c.salto('0f 85')
    c.salir(0)
    return c.terminar()


def consumidor():
    c = Codigo()
    reservar(c)
    c.recibir()
    c.comprobar('81 3b', 0)
    c.comprobar('81 7b 04', 0)
    c.servicio(9)
    c.comprobar('3d', 0)
    for valor in (1, 2, 3, 42):
        c.recibir()
        if valor != 42:
            c.comprobar('81 3b', 0)
        else:
            c.imm('81 3b', 0)
            c.salto('0f 84')              # El ultimo viene de un proceso real.
        c.comprobar('81 7b 04', valor)
    c.salir(42)
    return c.terminar()


def limites():
    c = Codigo()
    reservar(c)
    c.servicio(13)
    c.op('89 c6')                          # ID propio completo en ESI.
    sellar(c)
    # Espera de uno mismo sin plazo: consulta, no crea una dependencia.
    c.op('89 f3 89 e1')
    c.imm('ba', 0)
    c.servicio(15)
    c.comprobar('3d', 1)
    intacto(c)
    c.imm('ba', MAXIMO)
    c.servicio(15)
    c.comprobar('3d', 0xfffffffc)           # Espera positiva de si mismo: ciclo.
    intacto(c)
    for puntero in (0, 0x400000, 0x401ff9, 0xffffffff):
        c.imm('b9', puntero)
        c.imm('ba', 0)
        c.servicio(15)
        c.comprobar('3d', 0xffffffff)
        intacto(c)
    c.op('89 e1')
    for ticks in (0x80000000, 0xffffffff):
        c.imm('ba', ticks)
        c.servicio(15)
        c.comprobar('3d', 0xffffffff)
        intacto(c)
    c.imm('bb', 0)
    c.imm('ba', MAXIMO)
    c.servicio(15)
    c.comprobar('3d', 0xffffffff)
    intacto(c)
    # Autoenvio con espacio disponible no constituye un ciclo.
    c.op('89 f3')
    c.imm('b9', 42)
    c.imm('ba', MAXIMO)
    c.servicio(14)
    c.comprobar('3d', 0)
    c.op('89 e3')
    c.recibir()
    c.op('39 33')
    c.salto('0f 85')
    c.comprobar('81 7b 04', 42)
    for valor in range(4):
        c.op('89 f3')
        c.imm('b9', valor)
        c.servicio(11)
        c.comprobar('3d', 0)
    c.imm('ba', 0)
    c.servicio(14)
    c.comprobar('3d', 1)
    c.imm('ba', MAXIMO)
    c.servicio(14)
    c.comprobar('3d', 0xfffffffc)           # La cola propia llena no puede avanzar.
    for ticks in (0x80000000, 0xffffffff):
        c.imm('ba', ticks)
        c.servicio(14)
        c.comprobar('3d', 0xffffffff)
    c.imm('bb', 0)
    c.imm('ba', 0)
    c.servicio(14)
    c.comprobar('3d', 0xffffffff)
    c.op('89 e3')
    for valor in range(4):
        c.recibir()
        c.op('39 33')
        c.salto('0f 85')
        c.comprobar('81 7b 04', valor)
    c.salir(42)
    return c.terminar()


def antiguo():
    c = Codigo()
    for servicio in (14, 15):
        c.servicio(servicio)
        c.comprobar('3d', 0xfffffffe)
    c.salir(0)
    return c.terminar()


def programas_raw():
    # NXFS admite ocho entradas; ambos grupos usan volumenes separados.
    return {
        'enviar.nxe': emisor(), 'observar.nxe': observador(),
        'consumir.nxe': consumidor(), 'limites.nxe': limites(),
        'vivo.nxe': empaquetar(elf, bytes.fromhex('bb ff ff ff 7f b8 0a 00 00 00 cd 81 b8 07 00 00 00 cd 80')),
        'fin.nxe': empaquetar(elf, bytes.fromhex('b8 2a 00 00 00 cd 80')),
        'fallo.nxe': empaquetar(elf, bytes.fromhex('0f 0b')),
        'antiguo.nxe': antiguo()}


class Maquina:
    def __init__(self, carpeta, nombre, archivos):
        self.carpeta = carpeta
        self.datos = crear(archivos)
        self.disco = carpeta / (nombre + '.img')
        self.disco.write_bytes(self.datos)
        self.arranque = RAIZ / 'build/x86/nxk.img'
        self.original = self.arranque.read_bytes()
        self.log = carpeta / (nombre + '.log')
        self.salida = self.log.open('wb', buffering=0)
        self.p = subprocess.Popen([
            str(RAIZ / 'tools/qemu/qemu-system-i386.exe'), '-machine', 'pc', '-accel', 'tcg',
            '-cpu', 'qemu32', '-m', '32M', '-drive',
            f'file={self.arranque.as_posix()},format=raw,if=ide,index=0,snapshot=on',
            '-drive', f'file={self.disco.as_posix()},format=raw,if=ide,index=1,snapshot=on',
            '-display', 'none', '-serial', 'stdio', '-monitor', 'none', '-no-reboot'],
            cwd=RAIZ, stdin=subprocess.PIPE, stdout=self.salida, stderr=self.salida)

    def esperar(self, desde):
        limite = time.monotonic() + 20
        while time.monotonic() < limite:
            datos = self.log.read_bytes()[desde:]
            if b'nxk> ' in datos:
                return datos
            if self.p.poll() is not None:
                break
            time.sleep(.02)
        raise AssertionError(self.log.read_text(errors='replace'))

    def comando(self, texto, esperado=None):
        desde = self.log.stat().st_size
        for valor in (texto + '\n').encode():
            self.p.stdin.write(bytes([valor]))
            self.p.stdin.flush()
            time.sleep(.003)
        datos = self.esperar(desde)
        if esperado is not None:
            assert esperado in datos, (texto, datos)
        return datos

    def cargar(self, nombre):
        datos = self.comando('proceso cargar ' + nombre, b'Proceso ID ')
        return int(re.search(rb'Proceso ID (\d+)', datos)[1])

    def enviar(self, id, valor):
        self.comando(f'proceso enviar {id} {valor}', b'Mensaje enviado.')

    def llenar(self, id):
        for valor in range(4):
            self.enviar(id, valor)

    def configurar_emisor(self, destino, ticks=MAXIMO, valor=42):
        id = self.cargar('enviar.nxe')
        for dato in (destino, valor, ticks):
            self.enviar(id, dato)
        self.paso(id, 1)                   # Configuracion consumida, cesion inicial.
        return id

    def configurar_observador(self, destino, estado=3, codigo=42, ticks=MAXIMO):
        id = self.cargar('observar.nxe')
        for dato in (destino, ticks, estado, codigo):
            self.enviar(id, dato)
        return id

    def estados(self):
        datos = self.comando('proceso listar')
        campos = ('estado turnos cesiones preempciones dormidas despertar mensajes '
                  'enviados recibidos esperas plazos esperando esperas_envio esperas_fin ciclos').split()
        patron = rb'Proceso (\d+)' + b''.join(b' ' + c.encode() + rb' (\d+)' for c in campos)
        return {int(fila[0]): dict(zip(campos, map(int, fila[1:])))
                for fila in re.findall(patron, datos)}

    def info(self, id):
        estados = self.estados()
        assert id in estados, (id, estados, self.log.read_text(errors='replace'))
        return estados[id]

    def paso(self, id, estado, codigo=None):
        datos = self.comando(f'proceso paso {id}', f'Proceso {id} estado {estado}'.encode())
        if codigo is not None:
            assert f'normal, salida {codigo & 0xffffffff}'.encode() in datos, datos
        return datos

    def cancelar(self, id):
        self.comando(f'proceso cancelar {id}', b'Proceso cancelado.')

    def liberar(self, id, exito=True, antigua=False):
        nombre = 'usuario' if antigua else 'proceso'
        esperado = (b'Programa privado liberado.' if antigua else b'Proceso liberado.') if exito else (
            b'No se pudo liberar programa.' if antigua else b'No se pudo liberar proceso.')
        self.comando(f'{nombre} liberar {id}', esperado)

    def retenido(self, id):
        self.liberar(id, False)
        self.liberar(id, False, True)

    def cerrar(self):
        self.p.terminate()
        self.p.wait(timeout=5)
        self.salida.close()
        assert self.disco.read_bytes() == self.datos
        assert self.arranque.read_bytes() == self.original


def probar_raw(carpeta):
    m = Maquina(carpeta, 'dependencias', programas_raw())
    try:
        m.esperar(0)
        m.comando('volumen montar', b'Volumen NXFS montado')
        # Calentar las reservas de lectura y las dos rutas CPL3 antes de medir.
        id = m.cargar('fin.nxe')
        m.paso(id, 3, 42)
        m.liberar(id)
        m.comando('ejecutar usuario antiguo.nxe', b'Aplicacion NXE2 termino; consola recuperada.')
        m.comando('usuario estado', b'normal, salida 0')
        m.comando('usuario nextry', b'Aplicacion NEXTRY protegida termino.')
        m.comando('proceso dependencias', b'Dependencias: resultado42 recibido y liberacion anticipada protegida.')
        memoria = m.comando('mem')

        id = m.cargar('limites.nxe')
        m.paso(id, 3, 42)
        assert m.info(id)['ciclos'] == 2
        assert m.info(id)['esperas_envio'] == m.info(id)['esperas_fin'] == 0
        m.liberar(id)

        # Backpressure: otro proceso abre un lugar y el envio conserva la FIFO.
        destino = m.cargar('consumir.nxe')
        m.llenar(destino)
        id = m.configurar_emisor(destino)
        m.paso(id, 8)
        antes = m.info(id)
        assert antes['esperando'] == destino and antes['esperas_envio'] == 1
        ocupada = m.comando('mem')
        for _ in range(2):
            m.comando(f'proceso paso {id}', b'No se pudo ejecutar proceso.')
        assert m.info(id) == antes and m.comando('mem') == ocupada
        m.paso(destino, 1)
        m.paso(id, 3, 0)
        assert m.info(id)['enviados'] == 1 and m.info(id)['esperando'] == 0
        m.paso(destino, 3, 42)
        assert m.info(destino)['recibidos'] == 5
        m.liberar(id)
        m.liberar(destino)

        # El lugar que desperto al emisor no se reserva. La consola puede
        # ocuparlo antes del despacho sin crear un ciclo: volver a esperar
        # conserva plazo, contador de espera y turnos de CPU originales.
        destino = m.cargar('consumir.nxe')
        m.llenar(destino)
        id = m.configurar_emisor(destino)
        m.paso(id, 8)
        m.paso(destino, 1)
        assert m.info(id)['estado'] == 1
        m.enviar(destino, 99)
        antes = m.info(id)
        assert antes['estado'] == 1 and antes['ciclos'] == 0
        m.paso(id, 8)
        despues = m.info(id)
        assert despues['estado'] == 8 and despues['esperando'] == destino
        for campo in ('turnos', 'despertar', 'esperas_envio', 'plazos', 'enviados', 'ciclos'):
            assert despues[campo] == antes[campo], (campo, antes, despues)
        assert m.info(destino)['mensajes'] == 4
        m.cancelar(id)
        m.cancelar(destino)
        m.liberar(id)
        m.liberar(destino)

        # Sin lugar: consulta0 y plazo positivo devuelven1, sin enviar ni perder datos.
        for ticks in (0, 30):
            destino = m.cargar('vivo.nxe')
            m.llenar(destino)
            id = m.configurar_emisor(destino, ticks)
            if ticks:
                m.paso(id, 8)
                m.comando('proceso ejecutar', b'Procesos esperando plazo')
                # El destino solo duerme; su cola sigue llena.
                time.sleep(.6)
            m.paso(id, 3, 1)
            assert m.info(destino)['mensajes'] == 4
            assert m.info(id)['enviados'] == 0
            assert m.info(id)['plazos'] == int(bool(ticks))
            m.liberar(id)
            m.cancelar(destino)
            m.liberar(destino)

        # Espacio disponible antes del despacho gana aunque haya vencido el plazo.
        destino = m.cargar('consumir.nxe')
        m.llenar(destino)
        id = m.configurar_emisor(destino, 30)
        m.paso(id, 8)
        time.sleep(.6)
        m.paso(destino, 1)
        m.paso(id, 3, 0)
        assert m.info(id)['plazos'] == 0
        m.paso(destino, 3, 42)
        m.liberar(id)
        m.liberar(destino)

        # Destruir destino no retiene un envio: ID viejo no se entrega a otro slot.
        for cancelar in (False, True):
            destino = m.cargar('vivo.nxe')
            m.llenar(destino)
            id = m.configurar_emisor(destino)
            m.paso(id, 8)
            if cancelar:
                m.cancelar(destino)
                m.paso(id, 3, -3)
                m.liberar(destino)
            else:
                m.liberar(destino, antigua=True)
                nuevo = m.cargar('vivo.nxe')
                assert nuevo != destino
                m.paso(id, 3, -1)
                assert m.info(nuevo)['mensajes'] == 0
                m.cancelar(nuevo)
                m.liberar(nuevo)
            m.liberar(id)

        # El resultado terminal se observa varias veces, sin consumirlo.
        for archivo, estado, codigo in [('fin.nxe', 3, 42), ('fallo.nxe', 4, 0x80000006),
                                        ('vivo.nxe', 7, 0xffffffff)]:
            destino = m.cargar(archivo)
            if estado == 7:
                m.cancelar(destino)
            else:
                datos = m.paso(destino, estado, codigo if estado == 3 else None)
                if estado == 4:
                    assert b'fallo, vector 6' in datos
            for _ in range(2):
                id = m.configurar_observador(destino, estado, codigo)
                m.paso(id, 3, 0)
                assert m.info(id)['esperas_fin'] == 0
                m.liberar(id)
            m.liberar(destino)

        # Dos observadores pendientes retienen el resultado, incluso tras despertar.
        destino = m.cargar('fin.nxe')
        a = m.configurar_observador(destino)
        b = m.configurar_observador(destino)
        m.paso(a, 9)
        m.paso(b, 9)
        m.retenido(destino)
        m.paso(destino, 3, 42)
        m.retenido(destino)
        m.paso(a, 3, 0)
        m.retenido(destino)
        m.paso(b, 3, 0)
        m.liberar(destino)
        for id in (a, b):
            assert m.info(id)['esperas_fin'] == 1 and m.info(id)['esperando'] == 0
            m.liberar(id)

        # Un fallo o cancelacion del destino tambien despierta observadores,
        # conservando su resultado hasta entregar la copia completa.
        for archivo, estado, codigo in [('fallo.nxe', 4, 0x80000006),
                                        ('vivo.nxe', 7, 0xffffffff)]:
            destino = m.cargar(archivo)
            id = m.configurar_observador(destino, estado, codigo)
            m.paso(id, 9)
            if estado == 7:
                m.cancelar(destino)
            else:
                m.paso(destino, 4)
            m.retenido(destino)
            m.paso(id, 3, 0)
            m.liberar(destino)
            m.liberar(id)

        # Vacio/plazo conserva ambas salidas; una espera vencida aun retiene hasta despacho.
        for ticks in (0, 30):
            destino = m.cargar('vivo.nxe')
            id = m.configurar_observador(destino, ticks=ticks)
            if ticks:
                m.paso(id, 9)
                m.retenido(destino)
                time.sleep(.6)
                assert m.info(id)['estado'] == 1
                m.retenido(destino)
            m.paso(id, 3, 1)
            assert m.info(id)['plazos'] == int(bool(ticks))
            m.liberar(destino)
            m.liberar(id)

        # Resultado disponible despues del plazo pero antes de despacho tiene prioridad.
        destino = m.cargar('fin.nxe')
        id = m.configurar_observador(destino, ticks=30)
        m.paso(id, 9)
        time.sleep(.6)
        m.paso(destino, 3, 42)
        m.paso(id, 3, 0)
        assert m.info(id)['plazos'] == 0
        m.liberar(destino)
        m.liberar(id)
        # Destruir al observador por la API de espacios invalida su referencia.
        destino = m.cargar('vivo.nxe')
        id = m.configurar_observador(destino)
        m.paso(id, 9)
        m.retenido(destino)
        m.liberar(id, antigua=True)
        m.liberar(destino)

        # Cancelar esperadores retira dependencias y no publica resultados parciales.
        destino = m.cargar('vivo.nxe')
        id = m.configurar_observador(destino)
        m.paso(id, 9)
        m.retenido(destino)
        m.cancelar(id)
        assert m.info(id)['esperando'] == m.info(id)['despertar'] == 0
        m.liberar(destino)
        m.liberar(id)
        destino = m.cargar('vivo.nxe')
        m.llenar(destino)
        id = m.configurar_emisor(destino)
        m.paso(id, 8)
        m.cancelar(id)
        assert m.info(id)['esperando'] == m.info(id)['despertar'] == 0
        m.cancelar(destino)
        m.liberar(destino)
        m.liberar(id)

        # Ciclos de resultados de longitud2/3: solo la ultima arista se rechaza.
        for cantidad in (2, 3):
            ids = [m.cargar('observar.nxe') for _ in range(cantidad)]
            for indice, id in enumerate(ids):
                destino = ids[(indice + 1) % cantidad]
                codigo = 0xfffffffc if indice == cantidad - 2 else 0
                for dato in (destino, MAXIMO, 3, codigo):
                    m.enviar(id, dato)
            for id in ids[:-1]:
                m.paso(id, 9)
            m.paso(ids[-1], 3, -4)
            assert m.info(ids[-1])['ciclos'] == 1
            for id in reversed(ids[:-1]):
                m.paso(id, 3, 0)
            for id in ids:
                m.liberar(id)

        # Ciclo mixto FIN -> ENVIO lleno -> FIN.
        emisor = m.cargar('enviar.nxe')
        observador = m.configurar_observador(emisor, codigo=0xfffffffc)
        for dato in (observador, 42, MAXIMO):
            m.enviar(emisor, dato)
        m.paso(emisor, 1)
        m.paso(observador, 9)
        m.llenar(observador)
        m.paso(emisor, 3, -4)
        assert m.info(emisor)['ciclos'] == 1
        m.paso(observador, 3, 0)
        m.liberar(emisor)
        m.liberar(observador)

        # Dos envios llenos forman ciclo real tras consumir ambas configuraciones.
        a = m.cargar('enviar.nxe')
        b = m.cargar('enviar.nxe')
        for origen, destino in ((a, b), (b, a)):
            for dato in (destino, 42, MAXIMO):
                m.enviar(origen, dato)
            m.paso(origen, 1)
            m.llenar(origen)
        m.paso(a, 8)
        m.paso(b, 3, -4)
        m.paso(a, 3, -3)
        assert m.info(b)['ciclos'] == 1 and m.info(a)['enviados'] == 0
        m.liberar(a)
        m.liberar(b)

        # Al consumir la configuracion se abre espacio: no rechazar un ciclo resuelto.
        a = m.cargar('enviar.nxe')
        b = m.configurar_observador(a, codigo=0)
        for dato in (b, 42, MAXIMO):
            m.enviar(a, dato)
        m.paso(a, 1)
        m.paso(a, 8)
        m.paso(b, 9)
        assert m.info(b)['ciclos'] == 0
        m.paso(a, 3, 0)
        m.paso(b, 3, 0)
        assert m.info(b)['mensajes'] == 1
        m.liberar(a)
        m.liberar(b)

        # El hueco no esta reservado: otro envio puede cerrar un ciclo despues
        # de haber sido aceptada la segunda espera. Detectarlo al despachar,
        # sin esperar al plazo ni consumir el mensaje pendiente.
        a = m.cargar('enviar.nxe')
        b = m.configurar_observador(a, codigo=0xfffffffc)
        for dato in (b, 42, MAXIMO):
            m.enviar(a, dato)
        m.paso(a, 1)
        m.paso(a, 8)
        m.paso(b, 9)
        assert m.info(b)['ciclos'] == 0
        m.llenar(b)
        m.paso(a, 3, -4)
        assert m.info(a)['ciclos'] == 1 and m.info(a)['enviados'] == 0
        m.paso(b, 3, 0)
        assert m.info(b)['mensajes'] == 4
        m.liberar(a)
        m.liberar(b)

        assert m.comando('mem') == memoria
        m.comando('proceso listar', b'Sin procesos registrados.')
        m.comando('proceso dependencias', b'Dependencias: resultado42 recibido y liberacion anticipada protegida.')
        assert m.comando('mem') == memoria
        m.comando('proceso preempcion', b'Preempcion: bucle suspendido')
        m.comando('proceso dormir', b'Espera de proceso: otro avanzo')
        m.comando('tareas ocho', b'Ocho tareas: argumentos, pilas, esperas y finalizacion verificadas.')
    finally:
        m.cerrar()


def probar_nextry(carpeta):
    fuentes = {
        'nextry': '''funcion inicio
    entero propio = proceso_actual()
    entero valor = 7
    entero origen = 9
    entero estado = 17
    entero codigo = 19
    entero resultado = 23
    enviar_esperando propio, 42, resultado, 100
    afirmar resultado == 0
    recibir_mensaje valor, origen, resultado, 0
    afirmar resultado == 0 y origen == propio y valor == 42
    esperar_proceso propio, estado, codigo, resultado, 0
    afirmar resultado == 1 y estado == 17 y codigo == 19
    mostrar "NEXTRY: envio y consulta verificados"
    finalizar 42
fin
''',
        'observador': '''funcion inicio
    entero destino = 0
    entero origen = 0
    entero resultado = 0
    entero estado = 17
    entero codigo = 19
    recibir_mensaje destino, origen, resultado, 0
    afirmar resultado == 0 y origen == 0
    esperar_proceso destino, estado, codigo, resultado, 3000
    afirmar resultado == 0 y estado == 3 y codigo == 42
    mostrar "NEXTRY: resultado recibido"
    finalizar 0
fin
''',
        'productor': '''funcion inicio
    entero destino = 0
    entero origen = 0
    entero resultado = 0
    recibir_mensaje destino, origen, resultado, 0
    afirmar resultado == 0 y origen == 0
    enviar_esperando destino, 42, resultado, 3000
    afirmar resultado == 0
    mostrar "NEXTRY: envio reanudado"
    finalizar 43
fin
'''}
    # Ejecutar tambien los ejemplos publicados: las fuentes de usuario pueden
    # cambiar y deben seguir demostrando el flujo completo de doce mensajes.
    for nombre, fuente in {
            'ej_productor': 'proceso_productor.nxf',
            'ej_consumidor': 'proceso_consumidor.nxf',
            'ej_observador': 'proceso_observador.nxf'}.items():
        fuentes[nombre] = (RAIZ / 'lenguaje/ejemplos' / fuente).read_text(encoding='utf-8')
    archivos = {}
    entorno = {k: v for k, v in os.environ.items() if k.lower() != 'psmodulepath'}
    for nombre, texto in fuentes.items():
        fuente = carpeta / (nombre + '.nxf')
        destino = carpeta / (nombre + '.nxe')
        fuente.write_text(texto, encoding='utf-8')
        compilacion = subprocess.run([
            'powershell.exe', '-NoProfile', '-ExecutionPolicy', 'Bypass', '-File',
            str(RAIZ / 'compilar-usuario.ps1'), '-Fuente', str(fuente), '-Salida', str(destino)],
            cwd=RAIZ, env=entorno, capture_output=True, text=True)
        assert compilacion.returncode == 0, compilacion.stdout + compilacion.stderr
        archivos[nombre + '.nxe'] = destino.read_bytes()
    archivos['fin.nxe'] = programas_raw()['fin.nxe']
    archivos['consumir.nxe'] = consumidor()
    m = Maquina(carpeta, 'nextry-dependencias', archivos)
    try:
        m.esperar(0)
        m.comando('volumen montar', b'Volumen NXFS montado')
        id = m.cargar('nextry.nxe')
        datos = m.paso(id, 3, 42)
        assert b'NEXTRY: envio y consulta verificados' in datos
        m.liberar(id)
        destino = m.cargar('fin.nxe')
        id = m.cargar('observador.nxe')
        m.enviar(id, destino)
        m.paso(id, 9)
        m.paso(destino, 3, 42)
        datos = m.paso(id, 3, 0)
        assert b'NEXTRY: resultado recibido' in datos
        m.liberar(id)
        m.liberar(destino)
        destino = m.cargar('consumir.nxe')
        m.llenar(destino)
        id = m.cargar('productor.nxe')
        m.enviar(id, destino)
        m.paso(id, 8)
        m.paso(destino, 1)
        datos = m.paso(id, 3, 43)
        assert b'NEXTRY: envio reanudado' in datos
        m.paso(destino, 3, 42)
        m.liberar(id)
        m.liberar(destino)

        memoria = m.comando('mem')
        productor = m.cargar('ej_productor.nxe')
        consumidor_id = m.cargar('ej_consumidor.nxe')
        observador_id = m.cargar('ej_observador.nxe')
        m.enviar(productor, consumidor_id)
        m.enviar(observador_id, productor)
        m.paso(observador_id, 9)
        m.retenido(productor)
        # Forzar primero la contrapresion; las rondas posteriores deben
        # resolver todos los programas dentro del presupuesto de32 despachos.
        m.paso(productor, 8)
        datos = m.comando('proceso ejecutar', b'Procesos sin pendientes.')
        for marcador in (b'Productor: doce mensajes enviados',
                         b'Consumidor: suma comprobada\n78\n',
                         b'Observador: proceso terminado con codigo\n42\n'):
            assert marcador in datos, datos
        filas = m.estados()
        assert filas[productor]['estado'] == filas[consumidor_id]['estado'] == filas[observador_id]['estado'] == 3
        assert filas[productor]['enviados'] == filas[consumidor_id]['recibidos'] == 12
        assert filas[productor]['esperas_envio'] > 0
        assert filas[observador_id]['esperas_fin'] == 1
        listado = m.comando('proceso listar')
        for id, codigo in ((productor, 42), (consumidor_id, 42), (observador_id, 0)):
            patron = (rf'Proceso {id} estado 3[^\n]*\n'
                      rf'Ejecucion \d+: normal, salida {codigo}\r?\n').encode()
            assert re.search(patron, listado), (id, codigo, listado)
            assert filas[id]['esperando'] == filas[id]['despertar'] == 0
        for id in (observador_id, productor, consumidor_id):
            m.liberar(id)
        assert m.comando('mem') == memoria
    finally:
        m.cerrar()


def main():
    carpeta = RAIZ / 'build/x86/dependencias-pruebas'
    carpeta.mkdir(parents=True, exist_ok=True)
    probar_raw(carpeta)
    probar_nextry(carpeta)
    print('DEPENDENCIAS OK: backpressure FIFO, resultados, retencion, plazos, '
          'GPR/CF/DF, ciclos, cancelacion, IDs, NEXTRY y discos intactos.')


if __name__ == '__main__':
    main()
