"""CPL3 y NXFS en QEMU. Los ejecutables y discos se generan en build."""
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from pruebas_cuotas_procesos import compilar
from pruebas_dependencias import Maquina


RAIZ = Path(__file__).resolve().parents[1]
DISCO = 10
BASE = 0x402000
DENEGADO = 0xFFFFFFFB
ERROR = 0xFFFFFFFF

FUENTE = """funcion inicio
    entero base = reservar_paginas(2)
    afirmar base == 0x402000
    afirmar recibir_en_memoria(base, 0) == 0
    entero esperado = leer_entero(base + 4)
    entero destino = base + 4092
    escribir_entero destino, 0x12345678
    escribir_entero destino + 4, 0x12345678
    afirmar leer_archivo_en("dato.txt", destino, 16, 510) == esperado
    si esperado == 16
        afirmar leer_entero(destino) == 0x42414443
        afirmar leer_entero(destino + 4) == 0x42414443
    sino
        afirmar leer_entero(destino) == 0x12345678
        afirmar leer_entero(destino + 4) == 0x12345678
    fin
    afirmar liberar_paginas(base, 2) == 0
    mostrar "IO_RESULTADO"
    finalizar 42
fin
"""


def arrancar_lector(m, esperado):
    id = m.cargar('lector.nxe')
    m.enviar(id, esperado)
    return id


def terminar_lector(m, id):
    respuesta = m.comando('proceso ejecutar', b'Procesos sin pendientes.')
    assert b'IO_RESULTADO' in respuesta, respuesta
    assert m.info(id)['estado'] == 3
    m.liberar(id)


def main():
    carpeta = RAIZ / 'build/x86/lectura-diferida'
    carpeta.mkdir(parents=True, exist_ok=True)
    fuente = carpeta / 'lector.nxf'
    fuente.write_text(FUENTE, encoding='utf-8')
    datos = b'ABCD' * 1024
    ejemplos = RAIZ / 'lenguaje/ejemplos'
    archivos = {
        'lector.nxe': compilar(carpeta, 'lector', fuente),
        'otro.nxe': compilar(carpeta, 'otro', ejemplos / 'proceso.nxf'),
        'mostrar.nxe': compilar(carpeta, 'mostrar', ejemplos / 'usuario_volumen.nxf'),
        'dato.txt': datos,
        'leeme.txt': datos,
    }
    m = Maquina(carpeta, 'diferida', archivos)
    try:
        m.esperar(0)
        m.comando('volumen montar', b'Volumen NXFS montado')

        a = arrancar_lector(m, 16)
        b = arrancar_lector(m, 16)
        otro = m.cargar('otro.nxe')
        m.paso(a, DISCO)
        m.paso(otro, 1)
        m.paso(b, DISCO)
        m.comando('proceso resumen', b' disco 2')
        for base in (BASE, BASE + 4096):
            m.comando(f'proceso proteger {a} {base} 1 0', b'No se pudo cambiar proteccion.')
        respuesta = m.comando('proceso ejecutar', b'Procesos sin pendientes.')
        assert respuesta.count(b'IO_RESULTADO') == 2, respuesta
        assert b'Proceso NEXTRY: terminado' in respuesta
        for id in (a, b, otro):
            assert m.info(id)['estado'] == 3
            m.liberar(id)

        # Cancelar y liberar descarta la peticion y permite reutilizar el slot.
        a = arrancar_lector(m, 16)
        m.paso(a, DISCO)
        disco = m.comando('discos estado')
        m.cancelar(a)
        m.comando('proceso ejecutar', b'Procesos sin pendientes.')
        assert disco == m.comando('discos estado')
        m.liberar(a)

        b = arrancar_lector(m, DENEGADO)
        assert (a & 255) == (b & 255) and a != b
        m.paso(b, DISCO)
        m.comando(f'proceso permisos {b} 13', b'Permisos restringidos.')
        disco = m.comando('discos estado')
        terminar_lector(m, b)
        assert disco == m.comando('discos estado')

        a = arrancar_lector(m, DENEGADO)
        m.paso(a, DISCO)
        m.comando(f'proceso archivos {a} otro.txt', b'Archivos restringidos.')
        terminar_lector(m, a)

        a = arrancar_lector(m, ERROR)
        m.paso(a, DISCO)
        m.comando('volumen desmontar', b'Volumen NXFS desmontado.')
        disco = m.comando('discos estado')
        terminar_lector(m, a)
        assert disco == m.comando('discos estado')

        # Remontar antes de despachar tampoco recupera una peticion antigua.
        m.comando('volumen montar', b'Volumen NXFS montado')
        a = arrancar_lector(m, ERROR)
        m.paso(a, DISCO)
        m.comando('volumen desmontar', b'Volumen NXFS desmontado.')
        m.comando('volumen montar', b'Volumen NXFS montado')
        disco = m.comando('discos estado')
        terminar_lector(m, a)
        assert disco == m.comando('discos estado')

        a = m.cargar('mostrar.nxe')
        respuesta = m.comando('proceso ejecutar', b'Procesos sin pendientes.')
        assert datos in respuesta, respuesta
        assert m.info(a)['estado'] == 3
        m.liberar(a)
    finally:
        m.cerrar()
    print('NXFS CPL3 OK: dos lectores, concurrencia, destino cruzado, cancelacion, revocacion y 4 KiB.')


if __name__ == '__main__':
    main()
