"""Compara los bytes NXA con su contenido real en el ELF32 enlazado.
Detecta objetos .incbin obsoletos incluso si las direcciones no cambiaron.
"""
import struct,sys
from pathlib import Path
raiz=Path(sys.argv[1]); datos=(raiz/'kernel.elf').read_bytes()
if datos[:6]!=b'\x7fELF\x01\x01': raise SystemExit('Se requiere ELF32 little-endian')
pos=struct.unpack_from('<I',datos,32)[0]
tam,n=struct.unpack_from('<HH',datos,46)
secciones=[struct.unpack_from('<IIIIIIIIII',datos,pos+i*tam) for i in range(n)]
simbolos={}
for s in secciones:
    if s[1]!=2: continue
    t=secciones[s[6]]; nombres=datos[t[4]:t[4]+t[5]]
    for p in range(s[4],s[4]+s[5],s[9]):
        nombre,valor=struct.unpack_from('<II',datos,p)
        simbolos[nombres[nombre:].split(b'\0',1)[0].decode()]=valor
for nombre,archivo in (('usuario_nxf_inicio','usuario-nxf/programa.bin'),('kernel_entry','entrada.bin'),('reloj_nxa_inicio','reloj.bin'),
    ('servicio_nxa_inicio','servicio.bin'),('excepcion_nxa_inicio','excepciones.bin'),('nxk_cambiar_tarea','tareas.bin'),
    ('nxk_usuario_entrar','usuario_entrar.bin'),('nxk_usuario_salir','usuario_salir.bin'),
    ('nxk_usuario_abortar','usuario_abortar.bin'),('memset','llenar32.bin'),
    ('memcpy','copiar32.bin'),('memmove','mover32.bin')):
    esperado=(raiz/archivo).read_bytes(); direccion=simbolos[nombre]
    s=next(s for s in secciones if s[1]!=8 and s[3]<=direccion and direccion+len(esperado)<=s[3]+s[5])
    p=s[4]+direccion-s[3]
    if datos[p:p+len(esperado)]!=esperado: raise SystemExit('Objeto NXA obsoleto: '+nombre)
print('Binarios NEXTRY ASM enlazados verificados.')
