# GUIA DE LECTURA NXK - lenguaje/simbolos_elf.py
# Lee direcciones del archivo ELF enlazado para resolver referencias externas de la entrada NEXTRY ASM.
# Contexto y orden de estudio: ver LEER-CODIGO.md en la carpeta principal.
"""Extrae los simbolos ELF32 usados por la entrada NEXTRY ASM."""
import json,struct,argparse
from pathlib import Path
def simbolos(datos):
    if datos[:6]!=b'\x7fELF\x01\x01':raise ValueError('Se requiere ELF32 little-endian')
    tabla=struct.unpack_from('<I',datos,32)[0]
    tamano,cantidad=struct.unpack_from('<HH',datos,46)
    ss=[struct.unpack_from('<IIIIIIIIII',datos,tabla+i*tamano) for i in range(cantidad)]
    salida={}
    for s in ss:
        if s[1]!=2:continue
        nombres=ss[s[6]]; cadenas=datos[nombres[4]:nombres[4]+nombres[5]]
        for pos in range(s[4],s[4]+s[5],s[9]):
            nombre,valor=struct.unpack_from('<II',datos,pos)
            texto=cadenas[nombre:].split(b'\0',1)[0].decode()
            if texto in ('__bss_start','__bss_end','kernel_main','nxk_usuario_pila_guardada','nxk_usuario_cs_guardado'):salida[texto]=valor
    if len(salida)!=5:raise ValueError('Faltan simbolos del nucleo')
    return salida
if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('elf',type=Path);p.add_argument('json',type=Path);p.add_argument('--comprobar',action='store_true');a=p.parse_args()
    s=simbolos(a.elf.read_bytes())
    if a.comprobar:
        if s!=json.loads(a.json.read_text()):raise ValueError('El enlace final cambio las direcciones')
        print('Direcciones finales de NEXTRY ASM verificadas.')
    else:a.json.write_text(json.dumps(s))
