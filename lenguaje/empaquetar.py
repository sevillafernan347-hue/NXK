# GUIA DE LECTURA NXK - lenguaje/empaquetar.py
# Crea la cabecera NXE con arquitectura, tamanos, entrada y suma de comprobacion del programa.
# Contexto y orden de estudio: ver LEER-CODIGO.md en la carpeta principal.
"""Contenedor binario propio NXE 1, no un EXE de Windows renombrado."""
import argparse
import struct
from pathlib import Path


def empaquetar(elf, binario):
    if elf[:4] != b'\x7fELF' or elf[5] != 1:
        raise ValueError('El archivo intermedio debe ser ELF little-endian.')
    if elf[4] == 1:
        arquitectura = 1
        entrada, = struct.unpack_from('<I', elf, 24)
        tabla, = struct.unpack_from('<I', elf, 32)
        tamano, cantidad = struct.unpack_from('<HH', elf, 46)
        secciones = [struct.unpack_from('<IIIIIIIIII', elf, tabla + i*tamano) for i in range(cantidad)]
    elif elf[4] == 2:
        arquitectura = 2
        entrada, = struct.unpack_from('<Q', elf, 24)
        tabla, = struct.unpack_from('<Q', elf, 40)
        tamano, cantidad = struct.unpack_from('<HH', elf, 58)
        secciones = [struct.unpack_from('<IIQQQQIIQQ', elf, tabla + i*tamano) for i in range(cantidad)]
    else:
        raise ValueError('Arquitectura intermedia no reconocida.')
    base = 0x180000
    final = max(s[3]+s[5] for s in secciones if s[2] & 2)
    memoria = final - base
    if not (0 <= entrada-base < len(binario) <= memoria <= 128*1024):
        raise ValueError('Entrada, memoria o carga fuera de los limites de NXE 1.')
    suma = 2166136261
    for byte in binario:
        suma = ((suma ^ byte) * 16777619) & 0xffffffff
    cabecera = struct.pack('<4sHHIIIIII32s', b'NXE\0', 1, arquitectura, 64,
                          base, entrada-base, len(binario), memoria, suma, bytes(32))
    return cabecera + binario


if __name__ == '__main__':
    p = argparse.ArgumentParser(description='Crear un ejecutable nativo NXE.')
    p.add_argument('elf', type=Path); p.add_argument('binario', type=Path); p.add_argument('salida', type=Path)
    a = p.parse_args()
    a.salida.write_bytes(empaquetar(a.elf.read_bytes(), a.binario.read_bytes()))
    print(f'NXE creado: {a.salida}')
