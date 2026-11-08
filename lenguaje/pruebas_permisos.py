"""Permisos CPL3: reduccion irreversible y servicios denegados sin efectos.

Base2b8a2c9. Usa dos discos virtuales en snapshot y programas x86 acotados.
El grupo final tambien compila NEXTRY con las dos operaciones de permisos.
Requiere la imagen normal x86 v0.0.19 preparada por la integracion.
"""
import os
import pathlib
import re
import subprocess
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))
from pruebas_dependencias import (
    Codigo, Maquina, emisor, reservar, sellar, intacto, empaquetar, elf, MAXIMO)

RAIZ = pathlib.Path(__file__).resolve().parents[1]
DENEGADO = 0xfffffffb


def restringir(c, mascara, esperado=0):
    c.imm('bb', mascara)
    c.servicio(16)
    c.comprobar('3d', esperado)


def permisos(c, esperado):
    c.servicio(17)
    c.comprobar('3d', esperado)


def escribir_datos(c, direccion, datos):
    # Datos literales dentro de la pila propia, nunca punteros del kernel.
    for posicion in range(0, len(datos), 4):
        bloque = datos[posicion:posicion+4].ljust(4, b'\0')
        c.imm('c7 05', direccion + posicion)
        c.bytes.extend(bloque)


def actor_permisos():
    c = Codigo()
    reservar(c, 32)
    c.servicio(13)
    c.op('89 c6')
    permisos(c, 15)
    restringir(c, 16, 0xffffffff)
    permisos(c, 15)
    restringir(c, 7)
    restringir(c, 7)                    # La misma mascara es idempotente.
    permisos(c, 7)
    restringir(c, 15, DENEGADO)          # No puede recuperar el permiso IPC.
    permisos(c, 7)
    for numero in (11, 14):
        c.op('89 f3')
        c.imm('b9', 42)
        c.imm('ba', MAXIMO)
        c.servicio(numero)
        c.comprobar('3d', DENEGADO)
        c.op('39 f3')
        c.salto('0f 85')
    restringir(c, 0)
    permisos(c, 0)
    restringir(c, 0)
    restringir(c, 1, DENEGADO)
    # La salida denegada usa un rango valido y no imprime su marca.
    escribir_datos(c, 0x401fe0, b'NO-SALIDA')
    c.imm('bb', 0x401fe0)
    c.imm('b9', 9)
    c.servicio(1)
    c.comprobar('3d', DENEGADO)
    c.salir(42)
    return c.terminar()


def preparar_archivo(c):
    reservar(c, 64)
    escribir_datos(c, 0x401fc0, b'perm\0')
    escribir_datos(c, 0x401fd0, b'otro\0')
    escribir_datos(c, 0x401fe0, b'CAMBIO\0')


def pedir_archivo(c, numero, esperado):
    c.imm('bb', 0x401fc0)
    c.imm('b9', 0x401fd0 if numero in (7, 8) else 0x401fe0)
    c.imm('ba', 6)
    c.imm('be', 0)
    c.servicio(numero)
    c.comprobar('3d', esperado)


def actor_archivos():
    c = Codigo()
    preparar_archivo(c)
    restringir(c, 11)                   # Salida, lectura y envio; sin modificar.
    for numero in (2, 4, 6, 7, 8):
        pedir_archivo(c, numero, DENEGADO)
    # Leer sigue permitido y retorna los seis bytes que caben en el buffer.
    pedir_archivo(c, 5, 6)
    c.comprobar('81 3d e0 1f 40 00', int.from_bytes(b'orig', 'little'))
    restringir(c, 9)                    # Quitar tambien lectura.
    escribir_datos(c, 0x401fe0, b'INTACTO\0')
    for numero in (3, 5):
        pedir_archivo(c, numero, DENEGADO)
        c.comprobar('81 3d e0 1f 40 00', int.from_bytes(b'INTA', 'little'))
        c.comprobar('81 3d e4 1f 40 00', int.from_bytes(b'CTO\0', 'little'))
    permisos(c, 9)
    c.salir(42)
    return c.terminar()


def actor_copia_sin_lectura():
    c = Codigo()
    preparar_archivo(c)
    restringir(c, 5)                    # Puede modificar, pero copiar lee origen.
    pedir_archivo(c, 8, DENEGADO)
    permisos(c, 5)
    c.salir(42)
    return c.terminar()


def actor_antiguo():
    c = Codigo()
    for numero in (16, 17):
        c.imm('bb', 0)
        c.servicio(numero)
        c.comprobar('3d', 0xfffffffe)
    c.salir(0)
    return c.terminar()


def observador_pausado():
    c = Codigo()
    reservar(c, 24)
    for guardar in ('8b 73 04', '8b 43 04 89 44 24 10', '8b 7b 04', '8b 6b 04'):
        c.recibir()
        c.op(guardar)
    # La pausa permite llenar otra vez su cola antes de pedir FIN.
    c.servicio(9)
    c.comprobar('3d', 0)
    sellar(c)
    c.op('89 f3 89 e1 8b 54 24 10')
    c.servicio(15)
    c.comprobar('3d', 0)
    c.op('39 3c 24')
    c.salto('0f 85')
    c.op('39 6c 24 04')
    c.salto('0f 85')
    c.salir(0)
    return c.terminar()


def mascara(m, id):
    datos = m.comando('proceso listar')
    coincidencia = re.search(rb'Proceso ' + str(id).encode() + rb' estado [^\r\n]* permisos (\d+)', datos)
    assert coincidencia is not None, datos
    return int(coincidencia[1])


def limitar(m, id, valor, exito=True):
    m.comando(f'proceso permisos {id} {valor}',
              b'Permisos restringidos.' if exito else b'No se pudo restringir permisos.')


def archivos_intactos(m):
    datos = m.comando('leer perm', b'original')
    assert b'CAMBIO' not in datos, datos
    datos = m.comando('archivos')
    assert b'otro\r\n' not in datos and b'otro\n' not in datos, datos


def main():
    carpeta = RAIZ / 'build/x86/permisos-pruebas'
    carpeta.mkdir(parents=True, exist_ok=True)
    fuente = RAIZ/'lenguaje/ejemplos/proceso_permisos.nxf'
    entorno = {k: v for k, v in os.environ.items() if k.lower() != 'psmodulepath'}
    subprocess.run(['powershell.exe', '-NoProfile', '-ExecutionPolicy', 'Bypass', '-File',
                    str(RAIZ / 'compilar-usuario.ps1'), '-Fuente', str(fuente), '-Salida',
                    str(carpeta / 'nextry.nxe')], cwd=RAIZ, env=entorno, check=True, capture_output=True)
    archivos = {
        'permisos.nxe': actor_permisos(), 'archivos.nxe': actor_archivos(),
        'copia.nxe': actor_copia_sin_lectura(), 'antiguo.nxe': actor_antiguo(),
        'enviar.nxe': emisor(), 'observar.nxe': observador_pausado(),
        'vivo.nxe': empaquetar(elf, bytes.fromhex('bb ff ff ff 7f b8 0a 00 00 00 cd 81 b8 07 00 00 00 cd 80')),
        'nextry.nxe': (carpeta / 'nextry.nxe').read_bytes()}
    m = Maquina(carpeta, 'permisos', archivos)
    try:
        m.esperar(0)
        version = re.search(rb'#define NXK_VERSION "([^"]+)"', (RAIZ/'lib/version.h').read_bytes())[1]
        m.comando('version', version)
        m.comando('help', b'proceso permisos')
        m.comando('volumen montar', b'Volumen NXFS montado')
        m.comando('escribir perm original')
        m.comando('ejecutar usuario antiguo.nxe', b'Aplicacion NXE2 termino; consola recuperada.')
        id = m.cargar('permisos.nxe')
        assert mascara(m, id) == 15
        datos = m.paso(id, 3, 42)
        assert b'NO-SALIDA' not in datos, datos
        assert mascara(m, id) == 0 and m.info(id)['mensajes'] == 0
        limitar(m, id, 0, False)           # No alterar un terminal.
        m.liberar(id)
        memoria = m.comando('mem')

        for nombre, esperado in (('archivos.nxe', 9), ('copia.nxe', 5)):
            id = m.cargar(nombre)
            m.paso(id, 3, 42)
            assert mascara(m, id) == esperado
            archivos_intactos(m)
            m.liberar(id)

        # El operador kernel tambien puede reducir, nunca restaurar permisos.
        id = m.cargar('vivo.nxe')
        limitar(m, id, 16, False)
        limitar(m, id, 4294967296, False)
        assert mascara(m, id) == 15
        limitar(m, id, 7)
        limitar(m, id, 7)
        limitar(m, id, 15, False)
        m.paso(id, 5)
        limitar(m, id, 0)
        assert mascara(m, id) == 0
        m.enviar(id, 42)                  # El origen kernel0 no hereda restricciones.
        assert m.info(id)['mensajes'] == 1
        m.cancelar(id)
        limitar(m, id, 0, False)
        m.liberar(id)
        viejo = id
        id = m.cargar('vivo.nxe')
        assert id != viejo and mascara(m, id) == 15
        limitar(m, viejo, 0, False)
        assert mascara(m, id) == 15
        m.liberar(id)

        # Revocar permiso a un envio pendiente no entrega el valor retenido.
        destino = m.cargar('vivo.nxe')
        m.llenar(destino)
        id = m.configurar_emisor(destino)
        m.paso(id, 8)
        limitar(m, id, 7)
        assert m.info(id)['estado'] == 1
        m.paso(id, 3, -5)
        assert m.info(id)['enviados'] == 0 and m.info(id)['esperando'] == 0
        assert m.info(destino)['mensajes'] == 4
        m.liberar(id)
        m.liberar(destino)

        # Una arista SEND revocada ya no bloquea y no debe cerrar un ciclo FIN.
        a = m.cargar('enviar.nxe')
        b = m.configurar_observador(a, codigo=DENEGADO)
        for dato in (b, 42, MAXIMO):
            m.enviar(a, dato)
        m.paso(a, 1)
        m.paso(a, 8)
        m.paso(b, 1)                    # Consumir configuracion y abrir hueco.
        m.llenar(b)                     # Cerrarlo antes de registrar FIN.
        limitar(m, a, 7)
        m.paso(b, 9)
        assert m.info(b)['ciclos'] == 0
        m.paso(a, 3, -5)
        m.paso(b, 3, 0)
        m.liberar(a)
        m.liberar(b)

        id = m.cargar('nextry.nxe')
        datos = m.paso(id, 3, 42)
        assert b'Permisos: solo salida y lectura' in datos
        assert mascara(m, id) == 0
        m.liberar(id)
        archivos_intactos(m)
        assert m.comando('mem') == memoria
        m.comando('proceso listar', b'Sin procesos registrados.')
        m.comando('proceso dependencias', b'Dependencias: resultado42 recibido')
        assert m.comando('mem') == memoria
    finally:
        m.cerrar()
    print('PERMISOS OK: reduccion irreversible, FS/salida/IPC sin efectos, '
          'revocacion pendiente, ciclos, IDs, NEXTRY y memoria estable.')


if __name__ == '__main__':
    main()
