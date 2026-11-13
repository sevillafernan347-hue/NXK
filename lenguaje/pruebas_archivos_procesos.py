"""Lista de archivos por proceso, base1b4726b, contrato v0.0.22.

INT81/20 recibe EBX tabla de cantidad*24 bytes y ECX cantidad0..4.
Primera lista activa filtro; despues solo subconjuntos, lista0 irrevocable.
FS2..8 exige permisos globales y nombres incluidos, sin efectos al denegar.
Solo imagenes nuevas QEMU snapshot; requiere imagen normal integrada.
"""
import os
import pathlib
import re
import subprocess
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))
from pruebas_dependencias import Codigo, Maquina, empaquetar, elf

RAIZ = pathlib.Path(__file__).resolve().parents[1]
BASE = 0x402000
TABLA = BASE + 1000
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


def limitar(c, nombres=(), esperado=0, direccion=TABLA):
    for i, nombre in enumerate(nombres):
        datos(c, direccion + i * 24, nombre.encode() + b'\0')
    pedir_lista(c, direccion, len(nombres), esperado)


def pedir_lista(c, direccion, cantidad, esperado=0):
    c.imm('bb', direccion)
    c.imm('b9', cantidad)
    c.servicio(20)
    c.comprobar('3d', esperado)


def pausa(c):
    c.servicio(9)
    c.comprobar('3d', 0)


def archivo(c, servicio, nombre, buffer=0, cantidad=0, esperado=0):
    c.imm('bb', nombre)
    c.imm('b9', buffer)
    c.imm('ba', cantidad)
    c.imm('be', 0)
    c.servicio(servicio)
    c.comprobar('3d', esperado)


def actor_listas():
    c = Codigo()
    reservar(c)
    for cantidad in (5, 0xffffffff):
        pedir_lista(c, 0xffffffff, cantidad, ERROR)
    # Aunque el nombre termina antes del hueco, toda la entrada24 debe existir.
    datos(c, BASE + 8180, b'uno\0')
    for direccion in (0, 0x406000, BASE + 8180, 0xffffffff):
        pedir_lista(c, direccion, 1, ERROR)
    for nombre in ('', 'MAYUS', 'a/b'):
        limitar(c, (nombre,), ERROR)
    datos(c, TABLA, b'x' * 24)
    pedir_lista(c, TABLA, 1, ERROR)
    limitar(c, ('uno', 'uno'), ERROR)
    limitar(c, ('uno', 'a/b'), ERROR)
    limitar(c, ('uno', 'dos', 'tres', 'a/b'), ERROR)
    pausa(c)                            # Todos los rechazos conservan sin filtro.

    # La tabla puede estar en codigo RO: el kernel solo necesita leerla.
    c.salto('e9', 'tabla_fin')
    direccion_rom = 0x400000 + len(c.bytes)
    for nombre in ('uno', 'dos', 'tres', 'cuatro'):
        c.bytes.extend(nombre.encode().ljust(24, b'\0'))
    c.etiquetas['tabla_fin'] = len(c.bytes)
    pedir_lista(c, direccion_rom, 4)
    limitar(c, ('cuatro', 'tres', 'dos', 'uno'))
    pausa(c)                            # Reordenar no amplia ni pierde entradas.
    limitar(c, ('uno', 'uno'), ERROR)
    limitar(c, ('uno', 'otro'), DENEGADO)
    limitar(c, ('dos', 'uno'))
    pausa(c)
    # Tabla cruzada entre paginas reservadas, seguida de reduccion idempotente.
    limitar(c, ('uno',), direccion=BASE + 4090)
    # Los bytes posteriores a NUL no forman parte del nombre autorizado.
    datos(c, TABLA, b'uno\0BASURA\xff')
    pedir_lista(c, TABLA, 1)
    limitar(c, ('uno',))
    limitar(c, ('dos',), DENEGADO)
    pausa(c)
    pedir_lista(c, 0xffffffff, 0)         # Cero no lee el puntero.
    pedir_lista(c, 0, 0)
    limitar(c, ('uno',), DENEGADO)
    c.salir(42)
    return c.terminar()


def actor_archivos():
    c = Codigo()
    reservar(c)
    propio, ajeno, copia, nuevo = [BASE + i * 32 for i in range(4)]
    for direccion, nombre in ((propio, b'propio'), (ajeno, b'ajeno'),
                              (copia, b'copia'), (nuevo, b'nuevo')):
        datos(c, direccion, nombre + b'\0')
    fuente, buffer = BASE + 500, BASE + 700
    datos(c, fuente, b'CAMBIO!!')
    datos(c, buffer, b'INTACTO!')
    limitar(c, ('propio', 'copia', 'nuevo'))
    # Coincidencia exacta, incluso si el archivo no existe o procede de NXFS.
    for nombre in (b'propiox\0', b'fin.nxe\0'):
        datos(c, BASE + 160, nombre)
        archivo(c, 5, BASE + 160, buffer, 8, DENEGADO)
    for servicio in (2, 3, 4, 5, 6):
        archivo(c, servicio, ajeno, buffer, 8, DENEGADO)
        palabra(c, buffer, int.from_bytes(b'INTA', 'little'))
    # Ambos extremos deben estar autorizados antes de renombrar o copiar.
    for servicio in (7, 8):
        archivo(c, servicio, propio, ajeno, esperado=DENEGADO)
        archivo(c, servicio, ajeno, copia, esperado=DENEGADO)
    archivo(c, 5, propio, buffer, 8, 8)
    palabra(c, buffer, int.from_bytes(b'orig', 'little'))
    archivo(c, 8, propio, copia)
    archivo(c, 7, copia, nuevo)
    archivo(c, 4, nuevo)
    datos(c, fuente, b'++')
    archivo(c, 6, propio, fuente, 2)
    archivo(c, 3, propio, buffer, 10, 10)
    palabra(c, buffer + 8, int.from_bytes(b'++\0\0', 'little'))
    # Un filtro no es un permiso global: se intersectan las dos restricciones.
    c.imm('bb', 1)
    c.servicio(16)
    c.comprobar('3d', 0)
    for servicio in range(2, 9):
        archivo(c, servicio, 0xffffffff, 0xffffffff, 8, DENEGADO)
    pedir_lista(c, 0, 0)
    datos(c, fuente, b'ARCHIVOS-OK')
    c.imm('bb', fuente)
    c.imm('b9', 11)
    c.servicio(1)
    c.comprobar('3d', 11)
    c.salir(42)
    return c.terminar()


def actor_aislado():
    c = Codigo()
    c.op('83 ec 08 89 e3')
    c.recibir()
    c.op('8b 73 04')                      # 0=propio; 1=ajeno.
    reservar(c, 1)
    datos(c, BASE + 100, b'propio\0')
    datos(c, BASE + 132, b'ajeno\0')
    c.op('85 f6')
    c.salto('0f 85', 'elegir_ajeno')
    limitar(c, ('propio',))
    c.imm('bf', BASE + 100)
    c.imm('bd', BASE + 132)
    c.salto('e9', 'elegido')
    c.etiquetas['elegir_ajeno'] = len(c.bytes)
    limitar(c, ('ajeno',))
    c.imm('bf', BASE + 132)
    c.imm('bd', BASE + 100)
    c.etiquetas['elegido'] = len(c.bytes)
    pausa(c)
    c.op('89 fb')
    c.imm('b9', BASE + 300)
    c.imm('ba', 8)
    c.imm('be', 0)
    c.servicio(5)
    c.comprobar('3d', 8)
    c.op('89 eb')
    c.servicio(5)
    c.comprobar('3d', DENEGADO)
    c.salir(42)
    return c.terminar()


def actor_antiguo():
    c = Codigo()
    c.imm('bb', 0)
    c.imm('b9', 0)
    c.servicio(20)
    c.comprobar('3d', 0xfffffffe)
    c.salir(0)
    return c.terminar()


def politica(m, id):
    datos = m.comando('proceso listar')
    patron = (rb'Proceso ' + str(id).encode() + rb' estado [^\r\n]*'
              rb' archivos_restringidos (\d+) archivos_permitidos (\d+)')
    encontrado = re.search(patron, datos)
    assert encontrado is not None, datos
    return tuple(map(int, encontrado.groups()))


def compilar_ejemplo(carpeta):
    entorno = {k: v for k, v in os.environ.items() if k.lower() != 'psmodulepath'}
    salida = carpeta/'nextry.nxe'
    subprocess.run(['powershell.exe', '-NoProfile', '-ExecutionPolicy', 'Bypass', '-File',
                    str(RAIZ/'compilar-usuario.ps1'), '-Fuente',
                    str(RAIZ/'lenguaje/ejemplos/proceso_archivos.nxf'), '-Salida', str(salida)],
                   cwd=RAIZ, env=entorno, check=True, capture_output=True)
    return salida.read_bytes()


def main():
    carpeta = RAIZ/'build/x86/archivos-procesos-pruebas'
    carpeta.mkdir(parents=True, exist_ok=True)
    archivos = {'listas.nxe': actor_listas(), 'archivos.nxe': actor_archivos(),
                'aislado.nxe': actor_aislado(), 'antiguo.nxe': actor_antiguo(),
                'fin.nxe': empaquetar(elf, bytes.fromhex('b8 2a 00 00 00 cd 80')),
                'nextry.nxe': compilar_ejemplo(carpeta)}
    m = Maquina(carpeta, 'archivos-procesos', archivos)
    try:
        m.esperar(0)
        version = re.search(rb'#define NXK_VERSION "([^"]+)"', (RAIZ/'lib/version.h').read_bytes())[1]
        m.comando('version', version)
        m.comando('volumen montar', b'Volumen NXFS montado')
        m.comando('ejecutar usuario antiguo.nxe', b'Aplicacion NXE2 termino; consola recuperada.')
        id = m.cargar('fin.nxe')
        m.paso(id, 3, 42)
        m.liberar(id)
        m.comando('escribir calentar x')
        m.comando('borrar calentar')
        memoria = m.comando('mem')

        # El operador puede confinar antes de que el programa ejecute codigo.
        id = m.cargar('fin.nxe')
        for lista in ('a,b,c,d,e', 'a,', ',a', 'a,,b', 'a,a', 'A', 'x'*24):
            m.comando(f'proceso archivos {id} {lista}', b'No se pudo restringir archivos.')
            assert politica(m, id) == (0, 0)
        m.comando(f'proceso archivos {id} a,b,c,d', b'Archivos restringidos.')
        assert politica(m, id) == (1, 4)
        m.comando(f'proceso archivos {id} d,b', b'Archivos restringidos.')
        m.comando(f'proceso archivos {id} a', b'No se pudo restringir archivos.')
        assert politica(m, id) == (1, 2)
        m.comando(f'proceso archivos {id}', b'Archivos restringidos.')
        assert politica(m, id) == (1, 0)
        m.paso(id, 3, 42)
        m.comando(f'proceso archivos {id}', b'No se pudo restringir archivos.')
        m.liberar(id)
        m.comando(f'proceso archivos {id}', b'No se pudo restringir archivos.')
        m.comando('proceso archivos 4294967296 a', b'No se pudo restringir archivos.')
        id = m.cargar('fin.nxe')
        assert politica(m, id) == (0, 0)
        m.liberar(id)
        assert m.comando('mem') == memoria

        id = m.cargar('listas.nxe')
        assert politica(m, id) == (0, 0)
        for esperado in ((0, 0), (1, 4), (1, 2), (1, 1)):
            m.paso(id, 1)
            assert politica(m, id) == esperado
        m.paso(id, 3, 42)
        assert politica(m, id) == (1, 0)
        m.liberar(id)
        assert m.comando('mem') == memoria

        m.comando('escribir propio original')
        m.comando('escribir ajeno protegido')
        id = m.cargar('archivos.nxe')
        assert b'ARCHIVOS-OK' in m.paso(id, 3, 42)
        m.comando('leer propio', b'original++')
        m.comando('leer ajeno', b'protegido')
        assert politica(m, id) == (1, 0)
        m.liberar(id)
        # Restablecer longitud8 para ambos receptores independientes.
        m.comando('escribir propio original')
        m.comando('escribir ajeno distinto')
        a, b = m.cargar('aislado.nxe'), m.cargar('aislado.nxe')
        for id, seleccion in ((a, 0), (b, 1)):
            assert politica(m, id) == (0, 0)
            m.enviar(id, seleccion)
            m.paso(id, 1)
            assert politica(m, id) == (1, 1)
        m.paso(a, 3, 42)
        m.paso(b, 3, 42)
        m.liberar(a)
        m.liberar(b)
        id = m.cargar('aislado.nxe')
        m.enviar(id, 0)
        m.paso(id, 1)
        m.cancelar(id)
        assert politica(m, id) == (1, 1)
        viejo = id
        m.liberar(id)
        id = m.cargar('fin.nxe')
        assert id != viejo and politica(m, id) == (0, 0)
        m.liberar(viejo, False)
        m.liberar(id)
        for nombre in ('propio', 'ajeno'):
            m.comando('borrar ' + nombre)
        assert m.comando('mem') == memoria

        id = m.cargar('nextry.nxe')
        m.paso(id, 3, 42)
        assert politica(m, id) == (1, 0)
        m.comando('leer ajeno', b'intacto')
        m.liberar(id)
        for nombre in ('propio', 'copia', 'ajeno'):
            m.comando('borrar ' + nombre)
        assert m.comando('mem') == memoria
        m.comando('proceso listar', b'Sin procesos registrados.')
        historial = m.comando('usuario historial')
        m.comando('proceso resumen')
        assert m.comando('usuario historial') == historial
    finally:
        m.cerrar()
    print('ARCHIVOS PROCESOS OK: listas monotónicas, FS sin efectos al denegar, '
          'aislamiento, permisos, IDs, NEXTRY y memoria/discos estables.')


if __name__ == '__main__':
    main()
