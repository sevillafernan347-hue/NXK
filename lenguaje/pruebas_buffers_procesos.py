"""Buffers de archivos e IPC en paginas privadas, base ae5f335/v0.0.21.

Contrato aprobado por integracion antes de implementar: servicios1..8,12,15
aceptan datos respaldados en procesos gestionados. Validar todo el rango
antes de copiar/consumir; los huecos no producen efectos parciales. Esperas
conservan el buffer hasta despacho/cancelacion. La retencion contra liberar
paginas individuales se comprueba en las pruebas internas del kernel.

Solo imagenes nuevas de QEMU con snapshot; no accede a discos fisicos.
Requiere build/x86/nxk.img normal ya construido por integracion.
"""
import os
import pathlib
import re
import subprocess
import sys
import time

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))
from pruebas_dependencias import Codigo, Maquina, empaquetar, elf, MAXIMO

RAIZ = pathlib.Path(__file__).resolve().parents[1]
BASE = 0x402000
CRUCE = BASE + 4092
ERROR = 0xffffffff
DENEGADO = 0xfffffffb


def datos(c, direccion, contenido):
    for i in range(0, len(contenido), 4):
        c.imm('c7 05', direccion + i)
        c.bytes.extend(contenido[i:i + 4].ljust(4, b'\0'))


def palabra(c, direccion, valor):
    c.imm('81 3d', direccion)
    c.bytes.extend(valor.to_bytes(4, 'little'))
    c.salto('0f 85')


def reservar(c, cantidad=2):
    for i in range(cantidad):
        c.servicio(18)
        c.comprobar('3d', BASE + i * 4096)


def liberar(c, cantidad=2):
    for i in range(cantidad):
        c.imm('bb', BASE + i * 4096)
        c.servicio(19)
        c.comprobar('3d', 0)


def archivo(c, servicio, nombre, buffer=0, cantidad=0, esperado=0, posicion=0):
    c.imm('bb', nombre)
    c.imm('b9', buffer)
    c.imm('ba', cantidad)
    c.imm('be', posicion)
    c.servicio(servicio)
    c.comprobar('3d', esperado)


def sello(c):
    datos(c, CRUCE, bytes.fromhex('44 33 22 11 88 77 66 55'))


def intacto(c):
    palabra(c, CRUCE, 0x11223344)
    palabra(c, CRUCE + 4, 0x55667788)


def actor_archivos():
    c = Codigo()
    reservar(c, 3)
    nombre, copia, nuevo = BASE + 4093, BASE + 300, BASE + 320
    fuente, salida = BASE + 100, BASE + 8189
    datos(c, nombre, b'datos\0')            # Nombre atraviesa dos paginas.
    datos(c, copia, b'copia\0')
    datos(c, nuevo, b'nuevo\0')
    datos(c, fuente, b'NXK-DATO')
    archivo(c, 2, nombre, fuente, 8)
    archivo(c, 3, nombre, salida, 8, 8)     # Destino cruza otra frontera.
    palabra(c, salida, int.from_bytes(b'NXK-', 'little'))
    palabra(c, salida + 4, int.from_bytes(b'DATO', 'little'))
    archivo(c, 1, salida, 4, esperado=4)   # EBX=datos, ECX=cantidad.
    # Servicio1 no utiliza el contrato nombre/buffer de los archivos.
    c.imm('bb', salida)
    c.imm('b9', 8)
    c.servicio(1)
    c.comprobar('3d', 8)
    archivo(c, 5, nombre, salida, 4, 4, 4)
    palabra(c, salida, int.from_bytes(b'DATO', 'little'))
    # Tambien es valido cruzar de pila escribible a la primera pagina de datos.
    archivo(c, 3, nombre, 0x401ffc, 8, 8)
    palabra(c, 0x401ffc, int.from_bytes(b'NXK-', 'little'))
    palabra(c, BASE, int.from_bytes(b'DATO', 'little'))
    archivo(c, 6, nombre, fuente + 4, 4)
    archivo(c, 5, nombre, salida, 12, 12)
    palabra(c, salida + 8, int.from_bytes(b'DATO', 'little'))
    archivo(c, 8, nombre, copia)
    archivo(c, 7, copia, nuevo)
    archivo(c, 4, nuevo)

    # Conservar un nombre valido independiente antes de abrir un hueco.
    nombre = BASE + 500
    datos(c, nombre, b'datos\0')
    c.imm('bb', BASE + 4096)
    c.servicio(19)
    c.comprobar('3d', 0)
    datos(c, CRUCE, b'NO!!')
    for servicio in (2, 3, 5, 6):
        archivo(c, servicio, nombre, CRUCE, 8, ERROR)
        palabra(c, CRUCE, int.from_bytes(b'NO!!', 'little'))
    c.imm('bb', CRUCE)
    c.imm('b9', 8)
    c.servicio(1)
    c.comprobar('3d', ERROR)
    archivo(c, 4, CRUCE, esperado=ERROR)   # Nombre sin terminador en pagina valida.
    for puntero in (0, 0x400000, CRUCE, BASE + 12284, 0xffffffff):
        # Un buffer invalido no extrae el mensaje pendiente de la cola.
        c.imm('bb', puntero)
        c.imm('ba', 0)
        c.servicio(12)
        c.comprobar('3d', ERROR)
    c.servicio(13)
    c.op('89 c7')
    for puntero in (0, 0x400000, CRUCE, BASE + 12284, 0xffffffff):
        c.op('89 fb')
        c.imm('b9', puntero)
        c.imm('ba', 0)
        c.servicio(15)
        c.comprobar('3d', ERROR)
    c.imm('bb', BASE + 64)
    c.imm('ba', 0)
    c.servicio(12)
    c.comprobar('3d', 0)
    palabra(c, BASE + 64, 0)
    palabra(c, BASE + 68, 77)
    c.servicio(12)
    c.comprobar('3d', 1)
    palabra(c, BASE + 68, 77)
    # Verificar que los errores de escritura no alteraron el archivo original.
    archivo(c, 5, nombre, BASE + 800, 12, 12)
    for i, bloque in enumerate((b'NXK-', b'DATO', b'DATO')):
        palabra(c, BASE + 800 + i * 4, int.from_bytes(bloque, 'little'))
    c.imm('bb', 11)                       # Sin permiso de modificar archivos.
    c.servicio(16)
    c.comprobar('3d', 0)
    for servicio in (2, 4, 6, 7, 8):
        archivo(c, servicio, 0xffffffff, 0xffffffff, 8, DENEGADO)
    c.imm('bb', 9)                        # Sin lectura; deniega antes de punteros.
    c.servicio(16)
    c.comprobar('3d', 0)
    for servicio in (3, 5):
        archivo(c, servicio, 0xffffffff, 0xffffffff, 8, DENEGADO)
    c.imm('bb', 8)                        # Sin salida tampoco.
    c.servicio(16)
    c.comprobar('3d', 0)
    c.imm('bb', 0xffffffff)
    c.imm('b9', 8)
    c.servicio(1)
    c.comprobar('3d', DENEGADO)
    c.salir(42)
    return c.terminar()


def actor_recibir():
    c = Codigo()
    c.op('83 ec 10 89 e3')
    # Configuracion desde pila: ticks, status esperado y valor esperado.
    for guardar in ('8b 73 04', '8b 7b 04', '8b 6b 04'):
        c.recibir()
        c.op(guardar)
    reservar(c)
    sello(c)
    c.servicio(9)
    c.comprobar('3d', 0)
    c.imm('bb', CRUCE)
    c.op('89 f2')
    c.servicio(12)
    c.op('39 f8')
    c.salto('0f 85')
    c.op('85 c0')
    c.salto('0f 85', 'vacio')
    palabra(c, CRUCE, 0)
    c.imm('39 2d', CRUCE + 4)
    c.salto('0f 85')
    c.salto('e9', 'fin')
    c.etiquetas['vacio'] = len(c.bytes)
    intacto(c)
    c.etiquetas['fin'] = len(c.bytes)
    liberar(c)
    c.salir(42)
    return c.terminar()


def actor_observar():
    c = Codigo()
    c.op('83 ec 10 89 e3')
    # Destino, ticks, status y codigo terminal esperado.
    for guardar in ('8b 73 04', '8b 7b 04', '8b 6b 04', '8b 43 04 89 44 24 0c'):
        c.recibir()
        c.op(guardar)
    reservar(c)
    sello(c)
    c.servicio(9)
    c.comprobar('3d', 0)
    c.op('89 f3 89 fa')
    c.imm('b9', CRUCE)
    c.servicio(15)
    c.op('39 e8')
    c.salto('0f 85')
    c.op('85 c0')
    c.salto('0f 85', 'vacio')
    palabra(c, CRUCE, 3)
    c.op('8b 44 24 0c')
    c.imm('39 05', CRUCE + 4)
    c.salto('0f 85')
    c.salto('e9', 'fin')
    c.etiquetas['vacio'] = len(c.bytes)
    intacto(c)
    c.etiquetas['fin'] = len(c.bytes)
    liberar(c)
    c.salir(42)
    return c.terminar()


def actor_antiguo():
    c = Codigo()
    c.imm('bb', BASE)
    c.imm('b9', 8)
    c.servicio(1)
    c.comprobar('3d', ERROR)
    for servicio in (12, 15, 18, 19):
        c.servicio(servicio)
        c.comprobar('3d', 0xfffffffe)
    c.salir(0)
    return c.terminar()


def configurar(m, nombre, valores):
    id = m.cargar(nombre)
    for valor in valores:
        m.enviar(id, valor)
    m.paso(id, 1)
    return id


def compilar_ejemplo(carpeta, nombre):
    entorno = {k: v for k, v in os.environ.items() if k.lower() != 'psmodulepath'}
    salida = carpeta / (nombre + '.nxe')
    subprocess.run(['powershell.exe', '-NoProfile', '-ExecutionPolicy', 'Bypass', '-File',
                    str(RAIZ/'compilar-usuario.ps1'), '-Fuente',
                    str(RAIZ/'lenguaje/ejemplos'/ (nombre + '.nxf')), '-Salida', str(salida)],
                   cwd=RAIZ, env=entorno, check=True, capture_output=True)
    return salida.read_bytes()


def main():
    carpeta = RAIZ/'build/x86/buffers-procesos-pruebas'
    carpeta.mkdir(parents=True, exist_ok=True)
    archivos = {
        'archivos.nxe': actor_archivos(), 'recibir.nxe': actor_recibir(),
        'observar.nxe': actor_observar(), 'antiguo.nxe': actor_antiguo(),
        'fin.nxe': empaquetar(elf, bytes.fromhex('b8 2a 00 00 00 cd 80')),
        'vivo.nxe': empaquetar(elf, bytes.fromhex('bb ff ff ff 7f b8 0a 00 00 00 cd 81 0f 0b')),
        'nxfarch.nxe': compilar_ejemplo(carpeta, 'buffer_archivos'),
        'nxfipc.nxe': compilar_ejemplo(carpeta, 'memoria_mensajes')}
    m = Maquina(carpeta, 'buffers-procesos', archivos)
    try:
        m.esperar(0)
        version = re.search(rb'#define NXK_VERSION "([^"]+)"', (RAIZ/'lib/version.h').read_bytes())[1]
        m.comando('version', version)
        m.comando('volumen montar', b'Volumen NXFS montado')
        m.comando('ejecutar usuario antiguo.nxe', b'Aplicacion NXE2 termino; consola recuperada.')
        # Calentar carga, paso y liberacion antes de medir el heap.
        id = m.cargar('fin.nxe')
        m.paso(id, 3, 42)
        m.liberar(id)
        # RAMFS reserva su tabla una sola vez al crear el primer archivo.
        m.comando('escribir calentar x')
        m.comando('borrar calentar')
        memoria = m.comando('mem')
        id = m.cargar('archivos.nxe')
        m.enviar(id, 77)
        datos = m.paso(id, 3, 42)
        assert b'NXK-DATO' in datos and b'NO!!' not in datos, datos
        assert m.info(id)['mensajes'] == 0
        m.comando('leer datos', b'NXK-DATODATO')
        m.comando('borrar datos')
        m.liberar(id)
        assert m.comando('mem') == memoria

        # Recepcion inmediata, consulta vacia, espera real, plazo y prioridad.
        for ticks, esperado, valor, modo in ((0, 0, 71, 'inmediato'),
                (0, 1, 0, 'vacio'), (MAXIMO, 0, 72, 'esperar'),
                (25, 1, 0, 'plazo'), (25, 0, 73, 'tarde')):
            id = configurar(m, 'recibir.nxe', (ticks, esperado, valor))
            if modo == 'inmediato':
                m.enviar(id, valor)
            elif modo != 'vacio':
                m.paso(id, 6)
                if modo in ('plazo', 'tarde'):
                    time.sleep(.6)
                if modo in ('esperar', 'tarde'):
                    m.enviar(id, valor)
            m.paso(id, 3, 42)
            m.liberar(id)
            assert m.comando('mem') == memoria
        # Retirar el receptor, incluso por API legacy, no deja reservas/referencias.
        for antigua in (False, True):
            id = configurar(m, 'recibir.nxe', (MAXIMO, 0, 1))
            m.paso(id, 6)
            if not antigua:
                m.cancelar(id)
            m.liberar(id, antigua=antigua)
            assert m.comando('mem') == memoria

        # Resultado inmediato y entrega suspendida en buffer de dos paginas.
        for inmediato in (False, True):
            destino = m.cargar('fin.nxe')
            if inmediato:
                m.paso(destino, 3, 42)
            id = configurar(m, 'observar.nxe', (destino, MAXIMO, 0, 42))
            if not inmediato:
                m.paso(id, 9)
                m.retenido(destino)
                m.paso(destino, 3, 42)
            m.paso(id, 3, 42)
            m.liberar(id)
            m.liberar(destino)
            assert m.comando('mem') == memoria
        # FIN sin resultado: plazo conserva ambos enteros y suelta el objetivo.
        for ticks in (0, 25):
            destino = m.cargar('vivo.nxe')
            id = configurar(m, 'observar.nxe', (destino, ticks, 1, 0))
            if ticks:
                m.paso(id, 9)
                m.retenido(destino)
                time.sleep(.6)
            m.paso(id, 3, 42)
            m.liberar(destino)
            m.liberar(id)
            assert m.comando('mem') == memoria
        for antigua in (False, True):
            destino = m.cargar('fin.nxe')
            id = configurar(m, 'observar.nxe', (destino, MAXIMO, 0, 42))
            m.paso(id, 9)
            if not antigua:
                m.cancelar(id)
            m.liberar(id, antigua=antigua)
            m.liberar(destino)
            assert m.comando('mem') == memoria

        id = m.cargar('nxfarch.nxe')
        m.paso(id, 3, 42)
        m.liberar(id)
        id = m.cargar('nxfipc.nxe')
        # El ejemplo recibe en una pagina privada un mensaje del operador.
        m.enviar(id, 42)
        m.paso(id, 3, 42)
        m.liberar(id)
        assert m.comando('mem') == memoria
        m.comando('proceso listar', b'Sin procesos registrados.')
        historial = m.comando('usuario historial')
        m.comando('proceso resumen')
        assert m.comando('usuario historial') == historial
    finally:
        m.cerrar()
    print('BUFFERS PROCESOS OK: FS e IPC en datos privados, cruces/huecos, '
          'rechazo sin efectos, plazos/resultados, limpieza, NEXTRY y discos intactos.')


if __name__ == '__main__':
    main()
