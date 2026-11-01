# GUIA DE LECTURA NXK - lenguaje/nxj.py
# Interfaz experimental inspirada en Java: transforma clases con metodos estaticos al lenguaje base; no es Java completo.
# Contexto y orden de estudio: ver LEER-CODIGO.md en la carpeta principal.
"""NXJ 0.1: clases de metodos estaticos sobre NXK Espanol."""
import sys
from pathlib import Path
sys.path.insert(0, str(Path(__file__).resolve().parent))
import re
from compilador import compilar, ErrorLenguaje

def compilar_nxj(texto):
    lineas = texto.splitlines()
    utiles = [i for i,l in enumerate(lineas) if l.strip() and not l.lstrip().startswith('#')]
    if not utiles:
        raise ErrorLenguaje('Falta una clase.')
    primero, ultimo = utiles[0], utiles[-1]
    if not re.fullmatch(r'clase\s+[^\W\d]\w*', lineas[primero].strip()):
        raise ErrorLenguaje('El archivo debe comenzar con «clase Nombre».', primero+1)
    if lineas[ultimo].strip() != 'fin':
        raise ErrorLenguaje('Falta el fin de la clase.', ultimo+1)
    lineas[primero] = ''; lineas[ultimo] = ''
    profundidad = 0
    for i in range(primero+1, ultimo):
        l = lineas[i].strip()
        if not l or l.startswith('#'): continue
        if profundidad == 0:
            m = re.fullmatch(r'publico\s+estatico\s+(?:funcion|función)\s+([^\W\d]\w*)', l)
            if not m:
                raise ErrorLenguaje('La clase solo admite «publico estatico funcion nombre».', i+1)
            lineas[i] = 'funcion ' + m.group(1)
            profundidad = 1
        elif re.match(r'(si|mientras|repetir)\b', l):
            profundidad += 1
        elif l == 'fin':
            profundidad -= 1
    if profundidad:
        raise ErrorLenguaje('Falta cerrar un metodo o bloque con fin.', ultimo+1)
    return compilar('\n'.join(lineas))

if __name__ == '__main__':
    import argparse
    p = argparse.ArgumentParser(description='Compilador NXJ 0.1 hacia C nativo para NXK.')
    p.add_argument('archivo', type=Path)
    p.add_argument('-o', required=True, type=Path)
    a = p.parse_args()
    try:
        c = compilar_nxj(a.archivo.read_text(encoding='utf-8-sig'))
        a.o.parent.mkdir(parents=True, exist_ok=True)
        a.o.write_text(c, encoding='utf-8')
    except ErrorLenguaje as e:
        print(f'{a.archivo}:{e.linea}:{e.columna}: error: {e}', file=sys.stderr)
        sys.exit(1)
    print('NXJ: compilacion correcta.')
