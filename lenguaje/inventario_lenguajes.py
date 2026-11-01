"""Inventario de fuentes versionadas; no depende de la clasificacion de GitHub."""
from pathlib import Path
import subprocess
raiz=Path(__file__).resolve().parents[1]
grupos={'.nxf':'NEXTRY', '.nxj':'NXJ (prototipo)', '.nxa':'NEXTRY ASM',
        '.c':'C (puentes y runtime)', '.h':'C (puentes y runtime)',
        '.S':'Ensamblador GNU', '.py':'Python (compiladores y pruebas)',
        '.ps1':'PowerShell', '.cmd':'Windows Batch', '.js':'JavaScript'}
cuentas={}
for nombre in subprocess.check_output(['git','ls-files','-z'],cwd=raiz).decode().split('\0'):
    p=raiz/nombre
    if p.suffix not in grupos or not p.is_file(): continue
    grupo=grupos[p.suffix]; n,b=cuentas.get(grupo,(0,0))
    cuentas[grupo]=(n+1,b+len(p.read_bytes().replace(b'\r\n',b'\n')))
s=['# Lenguajes del proyecto','',
   'Fuentes versionadas; bytes normalizados a LF. No mide avance del kernel.',
   'NEXTRY/NXJ se enumeran aqui porque GitHub Linguist no los reconoce como lenguajes propios.',
   'NEXTRY ASM se presenta como Assembly en las estadisticas de GitHub.','',
   '| Lenguaje | Archivos | Bytes |','| --- | ---: | ---: |']
for grupo,(n,b) in sorted(cuentas.items()):s.append(f'| {grupo} | {n} | {b} |')
s+=['','Regenerar: `tools/python/python.exe lenguaje/inventario_lenguajes.py`.']
(raiz/'LENGUAJES.md').write_text('\n'.join(s)+'\n',encoding='utf-8')
