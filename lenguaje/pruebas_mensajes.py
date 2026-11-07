"""IPC CPL3 real: FIFO, origen, espera, plazo, cancelacion y limites de usuario.

Usa exclusivamente dos imagenes virtuales QEMU con snapshot. Los NXE pequenos
comprueban la ABI sin depender de que el compilador esconda errores del kernel.
Tambien compila y ejecuta un programa NEXTRY con las tres operaciones nuevas.
Requiere haber compilado antes la imagen x86 normal (sin NXK_TEST).
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

raiz = pathlib.Path(__file__).resolve().parents[1]
carpeta = raiz / 'build/x86/mensajes-pruebas'
carpeta.mkdir(parents=True, exist_ok=True)
entorno = {k: v for k, v in os.environ.items() if k.lower() != 'psmodulepath'}

fuente = carpeta / 'mensajes.nxf'
fuente.write_text('''# El resultado 1 conserva valor y origen anteriores.
funcion inicio
    entero propio = proceso_actual()
    entero valor = 17
    entero origen = 19
    entero resultado = 23
    recibir_mensaje valor, origen, resultado, 0
    afirmar resultado == 1
    afirmar valor == 17
    afirmar origen == 19
    enviar_mensaje propio, 42
    recibir_mensaje valor, origen, resultado, 0
    afirmar resultado == 0
    afirmar valor == 42
    afirmar origen == propio
    mostrar "NEXTRY: mensajes verificados"
fin
''', encoding='utf-8')
subprocess.run([
    'powershell.exe', '-NoProfile', '-ExecutionPolicy', 'Bypass', '-File',
    str(raiz / 'compilar-usuario.ps1'), '-Fuente', str(fuente),
    '-Salida', str(carpeta / 'nextry.nxe')], cwd=raiz, env=entorno,
    check=True, capture_output=True)

elf = bytearray(52)
elf[:7] = b'\x7fELF\x01\x01\x01'
struct.pack_into('<HHII', elf, 16, 2, 3, 1, 0x400000)


class Codigo:
    """Emisor minimo de instrucciones fijas; no ensambla fuentes externas."""
    def __init__(self):
        self.bytes = bytearray()
        self.saltos = []
        self.etiquetas = {}

    def op(self, hexadecimales):
        self.bytes.extend(bytes.fromhex(hexadecimales))

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

    def servicio(self, n):
        self.imm('b8', n)
        self.op('cd 81')

    def recibir(self, plazo):
        self.imm('ba', plazo)
        self.servicio(12)

    def salir(self, valor):
        self.imm('b8', valor)
        self.op('fc cd 80')

    def terminar(self):
        self.etiquetas['fallo'] = len(self.bytes)
        self.op('fc 0f 0b')
        for pos, nombre in self.saltos:
            struct.pack_into('<i', self.bytes, pos, self.etiquetas[nombre] - pos - 4)
        return empaquetar(elf, self.bytes)


def buffer(c):
    c.op('83 ec 08 89 e3')                 # ESP -= 8; EBX = buffer de usuario.
    c.imm('c7 03', 0x11223344)
    c.imm('c7 43 04', 0x55667788)


def intacto(c):
    c.comprobar('81 3b', 0x11223344)
    c.comprobar('81 7b 04', 0x55667788)


def fifo():
    c = Codigo()
    buffer(c)
    for valor in (0, 17, 0xffffffff, 42):
        c.recibir(0)
        c.comprobar('3d', 0)
        c.comprobar('81 3b', 0)             # Consola: remitente reservado 0.
        c.comprobar('81 7b 04', valor)
    c.salir(42)
    return c.terminar()


def esperar_mensaje(plazo):
    c = Codigo()
    # Reservar tambien donde capturar las banderas inmediatamente al regresar.
    c.op('83 ec 0c 89 e3')
    c.imm('c7 03', 0x11223344)
    c.imm('c7 43 04', 0x55667788)
    for op, valor in [('b9', 0x22334455), ('be', 0x44556677),
                      ('bf', 0x55667788), ('bd', 0x66778899)]:
        c.imm(op, valor)
    c.imm('ba', plazo)
    c.op('f9 fd')                          # CF y DF deben sobrevivir al bloqueo.
    c.servicio(12)
    c.op('9c 8f 43 08 fc 50 8b 43 08')   # Capturar flags sin cambiar EAX.
    c.imm('25', 0x401)
    c.comprobar('3d', 0x401)
    c.op('58')
    for op, valor in [('81 fb', 0x401ff4), ('81 f9', 0x22334455),
                      ('81 fa', plazo), ('81 fe', 0x44556677),
                      ('81 ff', 0x55667788), ('81 fd', 0x66778899),
                      ('81 fc', 0x401ff4)]:
        c.comprobar(op, valor)
    c.imm('3d', 1)
    c.salto('0f 84', 'plazo')
    c.comprobar('3d', 0)
    c.comprobar('81 3b', 0)
    c.comprobar('81 7b 04', 42)
    c.salir(42)
    c.etiquetas['plazo'] = len(c.bytes)
    intacto(c)
    c.salir(43)
    return c.terminar()


def limites():
    c = Codigo()
    buffer(c)
    # Hay un mensaje esperando: rechazar estos contratos NO debe consumirlo.
    for puntero in (0, 0x400000, 0x401ff9, 0xffffffff):
        c.imm('bb', puntero)
        c.recibir(0)
        c.comprobar('3d', 0xffffffff)
    c.imm('bb', 0x401ff8)
    for plazo in (0x80000000, 0xffffffff):
        c.recibir(plazo)
        c.comprobar('3d', 0xffffffff)
    intacto(c)
    c.recibir(0)
    c.comprobar('3d', 0)
    c.comprobar('81 3b', 0)
    c.comprobar('81 7b 04', 42)
    c.imm('c7 03', 0x11223344)
    c.imm('c7 43 04', 0x55667788)
    c.recibir(0)
    c.comprobar('3d', 1)
    intacto(c)
    c.imm('bb', 0)
    c.imm('b9', 42)
    c.servicio(11)
    c.comprobar('3d', 0xffffffff)
    c.salir(42)
    return c.terminar()


def propio():
    c = Codigo()
    c.servicio(13)
    c.imm('3d', 0xfffffffe)
    c.salto('0f 84', 'antiguo')
    c.imm('3d', 0)
    c.salto('0f 84')
    c.op('89 c6')                          # ESI = ID completo del remitente.
    for valor in (0, 17, 0xffffffff, 42):
        c.op('89 f3')
        c.imm('b9', valor)
        c.servicio(11)
        c.comprobar('3d', 0)
    c.servicio(11)
    c.comprobar('3d', 0xfffffffe)           # Quinta entrada: cola llena.
    buffer(c)
    for valor in (0, 17):
        c.recibir(0)
        c.comprobar('3d', 0)
        c.op('39 33')
        c.salto('0f 85')
        c.comprobar('81 7b 04', valor)
    # Rellenar con dos mensajes aun pendientes fuerza el cruce del anillo:
    # no basta probar una cola vacia que siempre empieza por el indice cero.
    for valor in (63, 64):
        c.op('89 f3')
        c.imm('b9', valor)
        c.servicio(11)
        c.comprobar('3d', 0)
    c.op('89 e3')                          # Recuperar puntero al buffer privado.
    for valor in (0xffffffff, 42, 63, 64):
        c.recibir(0)
        c.comprobar('3d', 0)
        c.op('39 33')                      # El origen no se toma de argumentos.
        c.salto('0f 85')
        c.comprobar('81 7b 04', valor)
    c.salir(42)
    c.etiquetas['antiguo'] = len(c.bytes)
    # La ruta sincronica antigua no ofrece procesos: rechaza los tres servicios.
    c.imm('bb', 0x401ff8)
    c.imm('b9', 42)
    c.imm('ba', 0)
    c.servicio(11)
    c.comprobar('3d', 0xfffffffe)
    c.servicio(12)
    c.comprobar('3d', 0xfffffffe)
    c.salir(0)
    return c.terminar()


def entre_procesos(emisor):
    c = Codigo()
    buffer(c)
    c.recibir(0)
    c.comprobar('3d', 0)
    c.comprobar('81 3b', 0)
    c.op('8b 73 04')                       # ESI = ID del otro proceso.
    if emisor:
        c.op('89 f3')
        c.imm('b9', 42)
        c.servicio(11)
        c.comprobar('3d', 0)
    else:
        c.recibir(0x7fffffff)
        c.comprobar('3d', 0)
        c.op('39 33')
        c.salto('0f 85')
        c.comprobar('81 7b 04', 42)
    c.salir(42)
    return c.terminar()


datos = crear({
    'fifo.nxe': fifo(), 'espera.nxe': esperar_mensaje(0x7fffffff),
    'plazo.nxe': esperar_mensaje(30), 'limites.nxe': limites(),
    'propio.nxe': propio(), 'emisor.nxe': entre_procesos(True),
    'receptor.nxe': entre_procesos(False),
    'nextry.nxe': (carpeta / 'nextry.nxe').read_bytes()})
disco = carpeta / 'mensajes.img'
disco.write_bytes(datos)
arranque = raiz / 'build/x86/nxk.img'
original = arranque.read_bytes()
log = carpeta / 'consola.log'

with log.open('wb', buffering=0) as salida:
    p = subprocess.Popen([
        str(raiz / 'tools/qemu/qemu-system-i386.exe'), '-machine', 'pc', '-accel', 'tcg',
        '-cpu', 'qemu32', '-m', '32M', '-drive',
        f'file={arranque.as_posix()},format=raw,if=ide,index=0,snapshot=on', '-drive',
        f'file={disco.as_posix()},format=raw,if=ide,index=1,snapshot=on',
        '-display', 'none', '-serial', 'stdio', '-monitor', 'none', '-no-reboot'],
        cwd=raiz, stdin=subprocess.PIPE, stdout=salida, stderr=salida)

    def esperar(desde):
        limite = time.monotonic() + 20
        while time.monotonic() < limite:
            d = log.read_bytes()[desde:]
            if b'nxk> ' in d:
                return d
            if p.poll() is not None:
                break
            time.sleep(.02)
        raise AssertionError(log.read_text(errors='replace'))

    def comando(texto, esperado=None):
        desde = log.stat().st_size
        for v in (texto + '\n').encode():
            p.stdin.write(bytes([v]))
            p.stdin.flush()
            time.sleep(.003)
        d = esperar(desde)
        if esperado is not None:
            assert esperado in d, (texto, d)
        return d

    def cargar(nombre):
        d = comando('proceso cargar ' + nombre, b'Proceso ID ')
        return int(re.search(rb'Proceso ID (\d+)', d)[1])

    def liberar(id):
        comando(f'proceso liberar {id}', b'Proceso liberado.')

    def enviar(id, valor, exito=True):
        comando(f'proceso enviar {id} {valor}',
                b'Mensaje enviado.' if exito else b'No se pudo enviar mensaje.')

    def cancelar(id, exito=True):
        comando(f'proceso cancelar {id}',
                b'Proceso cancelado.' if exito else b'No se pudo cancelar proceso.')

    def estados():
        d = comando('proceso listar')
        filas = re.findall(
            rb'Proceso (\d+) estado (\d+) turnos (\d+) cesiones (\d+) '
            rb'preempciones (\d+) dormidas (\d+) despertar (\d+) '
            rb'mensajes (\d+) enviados (\d+) recibidos (\d+) esperas (\d+) plazos (\d+)', d)
        return {int(f[0]): tuple(map(int, f[1:])) for f in filas}, d

    def info(id):
        todas, texto = estados()
        assert id in todas, (id, texto)
        return todas[id]

    def paso(id, estado, codigo=None):
        d = comando(f'proceso paso {id}', f'Proceso {id} estado {estado}'.encode())
        if codigo is not None:
            assert f'normal, salida {codigo}'.encode() in d, d
        return d

    try:
        esperar(0)
        comando('volumen montar', b'Volumen NXFS montado')
        # Calentar las reservas de lectura, pila CPL3 y ruta antigua antes de mem.
        id = cargar('nextry.nxe')
        d = paso(id, 3, 0)
        assert b'NEXTRY: mensajes verificados' in d
        liberar(id)
        comando('ejecutar usuario propio.nxe', b'Aplicacion NXE2 termino; consola recuperada.')
        comando('usuario estado', b'normal, salida 0')
        memoria = comando('mem')

        # FIFO exacta, cero y UINT32_MAX, quinta entrada sin sobrescribir.
        id = cargar('fifo.nxe')
        for valor in (0, 17, 0xffffffff, 42):
            enviar(id, valor)
        antes = info(id)
        assert antes[6] == 4 and antes[8] == 0, antes
        enviar(id, 99, False)
        assert info(id) == antes
        paso(id, 3, 42)
        assert info(id)[6:11] == (0, 0, 4, 0, 0), info(id)
        enviar(id, 42, False)
        cancelar(id, False)
        liberar(id)

        # Rechazos de buffer/plazo no consumen ni alteran el mensaje pendiente.
        id = cargar('limites.nxe')
        enviar(id, 42)
        paso(id, 3, 42)
        assert info(id)[6:11] == (0, 0, 1, 0, 0), info(id)
        liberar(id)

        # El propio origen llega como ID completo; FIFO tambien desde CPL3.
        id = cargar('propio.nxe')
        paso(id, 3, 42)
        assert info(id)[6:11] == (0, 6, 6, 0, 0), info(id)
        liberar(id)

        # Esperar no gasta turnos ni memoria; enviar despierta sin ejecutar aun.
        id = cargar('espera.nxe')
        paso(id, 6)
        antes = info(id)
        assert antes[1] == 1 and antes[9:] == (1, 0), antes
        ocupada = comando('mem')
        for _ in range(3):
            comando('proceso ejecutar', b'Procesos esperando plazo')
            comando(f'proceso paso {id}', b'No se pudo ejecutar proceso.')
        assert info(id) == antes and comando('mem') == ocupada
        enviar(id, 42)
        assert info(id)[0] == 1 and info(id)[6] == 1
        paso(id, 3, 42)
        assert info(id)[6:11] == (0, 0, 1, 1, 0), info(id)
        liberar(id)

        # Plazo vencido: devuelve 1, buffer intacto y una sola espera registrada.
        id = cargar('plazo.nxe')
        paso(id, 6)
        time.sleep(.6)
        paso(id, 3, 43)
        assert info(id)[6:11] == (0, 0, 0, 1, 1), info(id)
        liberar(id)
        # Si llega antes de reanudar, el mensaje disponible tiene prioridad.
        id = cargar('plazo.nxe')
        paso(id, 6)
        time.sleep(.6)
        enviar(id, 42)
        paso(id, 3, 42)
        assert info(id)[6:11] == (0, 0, 1, 1, 0), info(id)
        liberar(id)

        # Dos procesos distintos: remitente autenticado y despertar del receptor.
        receptor = cargar('receptor.nxe')
        emisor = cargar('emisor.nxe')
        enviar(receptor, emisor)
        enviar(emisor, receptor)
        paso(receptor, 6)
        paso(emisor, 3, 42)
        paso(receptor, 3, 42)
        assert info(receptor)[6:11] == (0, 0, 2, 1, 0), info(receptor)
        assert info(emisor)[6:11] == (0, 1, 1, 0, 0), info(emisor)
        liberar(receptor)
        liberar(emisor)

        # Cancelar retiene el ID hasta liberar, limpia cola/plazo y no resucita.
        for esperando in (False, True):
            id = cargar('espera.nxe')
            if esperando:
                paso(id, 6)
            else:
                enviar(id, 42)
            ocupada = comando('mem')
            cancelar(id)
            estado = info(id)
            assert estado[0] == 7 and estado[5] == 0 and estado[6] == 0, estado
            assert comando('mem') == ocupada
            cancelar(id, False)
            enviar(id, 42, False)
            comando(f'proceso paso {id}', b'No se pudo ejecutar proceso.')
            comando('proceso ejecutar', b'Procesos sin pendientes.')
            assert info(id) == estado
            liberar(id)
            viejo = id
            id = cargar('propio.nxe')
            assert id != viejo
            enviar(viejo, 42, False)
            cancelar(viejo, False)
            assert info(id)[6:11] == (0, 0, 0, 0, 0)
            paso(id, 3, 42)
            liberar(id)

        # Liberar con la API antigua tambien invalida cola y espera pendientes.
        viejo = cargar('espera.nxe')
        paso(viejo, 6)
        comando(f'usuario liberar {viejo}', b'Programa privado liberado.')
        id = cargar('propio.nxe')
        assert id != viejo
        enviar(viejo, 42, False)
        cancelar(viejo, False)
        paso(id, 3, 42)
        liberar(id)
        enviar(0, 42, False)
        cancelar(0, False)
        comando('proceso enviar 1 4294967296', b'No se pudo enviar mensaje.')
        assert comando('mem') == memoria
        comando('usuario nextry', b'Aplicacion NEXTRY protegida termino.')
        comando('tareas ocho', b'Ocho tareas: argumentos, pilas, esperas y finalizacion verificadas.')
        comando('proceso dormir', b'Espera de proceso: otro avanzo y el dormido continuo al vencer.')
        comando('help', b'proceso cancelar')
    finally:
        p.terminate()
        p.wait(timeout=5)

assert disco.read_bytes() == datos and arranque.read_bytes() == original
print('MENSAJES OK: FIFO4, origen CPL3, espera/plazo, GPR/CF/DF, limites, cancelacion, IDs y memoria estable.')
