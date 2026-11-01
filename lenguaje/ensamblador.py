# GUIA DE LECTURA NXK - lenguaje/ensamblador.py
# Ensamblador propio: convierte instrucciones espanolas en bytes x86; dos pasadas resuelven etiquetas.
# Contexto y orden de estudio: ver LEER-CODIGO.md en la carpeta principal.
"""NEXTRY ASM 0.1: ensamblador binario propio de dos pasadas, x86 BIOS.
No invoca GNU as, NASM, Clang ni otro ensamblador.
"""
import ast
import argparse
import json
from pathlib import Path

FIJAS = {
    'desactivar_interrupciones': b'\xfa', 'activar_interrupciones': b'\xfb',
    'direccion_adelante': b'\xfc', 'detener': b'\xf4',
    'cero ax': b'\x31\xc0', 'segmento ds, ax': b'\x8e\xd8',
    'segmento es, ax': b'\x8e\xc0', 'segmento ss, ax': b'\x8e\xd0',
    'segmento fs, ax': b'\x8e\xe0', 'segmento gs, ax': b'\x8e\xe8',
    'leer_control eax, cr0': b'\x0f\x20\xc0',
    'escribir_control cr0, eax': b'\x0f\x22\xc0',
    'saltar_registro eax': b'\xff\xe0',
    'guardar_registros32': b'\x60',
    'recuperar_registros32': b'\x61',
    'retornar_interrupcion32': b'\xcf',
    'alinear_pila16': b'\x83\xe4\xf0',
    'llamar_registro eax': b'\xff\xd0',
    'cero eax': b'\x31\xc0',
    'restar32 ecx, edi': b'\x29\xf9',
    'llenar_bytes': b'\xf3\xaa',
    'copiar_bytes': b'\xf3\xa4',
}
REGISTROS = ['ax','cx','dx','bx','sp','bp','si','di']

def numero(texto, etiquetas, pendiente=False):
    # Interpreta solo enteros, etiquetas y sumas/restas: nunca ejecuta el texto.
    def visitar(n):
        if isinstance(n, ast.Constant) and type(n.value) is int: return n.value
        if isinstance(n, ast.Name):
            if n.id in etiquetas: return etiquetas[n.id]
            if pendiente: return 0
            raise ValueError('Etiqueta desconocida: '+n.id)
        if isinstance(n, ast.BinOp) and isinstance(n.op,(ast.Add,ast.Sub)):
            a,b=visitar(n.left),visitar(n.right)
            return a+b if isinstance(n.op,ast.Add) else a-b
        if isinstance(n,ast.UnaryOp) and isinstance(n.op,ast.USub):return -visitar(n.operand)
        raise ValueError('Expresion no admitida: '+texto)
    return visitar(ast.parse(texto,mode='eval').body)

def ensamblar(fuente, simbolos=None, permitir_externos=False):
    etiquetas=dict(simbolos or {})
    for pasada in range(2):
        # Primera pasada: descubre direcciones. Segunda: escribe saltos resueltos.
        datos=bytearray(); origen=0; modo=16
        for linea, original in enumerate(fuente.splitlines(),1):
            texto=original.split('#',1)[0].strip()
            if not texto:continue
            try:
                if texto.endswith(':'):
                    nombre=texto[:-1]
                    if not nombre.isidentifier():raise ValueError('Nombre de etiqueta incorrecto')
                    if pasada==0:
                        if nombre in etiquetas:raise ValueError('Etiqueta duplicada: '+nombre)
                        etiquetas[nombre]=origen+len(datos)
                    continue
                orden,_,argumento=texto.partition(' ')
                args=[a.strip() for a in argumento.split(',')]
                def valor(t):return numero(t,etiquetas,pasada==0)
                def emitir(n,tamano,signado=False):
                    # x86 almacena primero el byte menos significativo (little endian).
                    datos.extend(n.to_bytes(tamano,'little',signed=signado))
                if orden=='externo':
                    if argumento not in etiquetas:
                        if not permitir_externos:raise ValueError('Falta resolver el simbolo externo: '+argumento)
                        etiquetas[argumento]=0
                elif orden=='origen':
                    if datos:raise ValueError('origen debe preceder al codigo')
                    origen=valor(argumento)
                elif orden=='modo':
                    modo=valor(argumento)
                    if modo not in (16,32):raise ValueError('Solo se admiten modos 16 y 32')
                elif orden in ('bytes','palabra','doble','cuadruple'):
                    tamano={'bytes':1,'palabra':2,'doble':4,'cuadruple':8}[orden]
                    for a in args:emitir(valor(a),tamano)
                elif orden=='alinear':
                    n=valor(argumento)
                    if n<1 or n>4096:raise ValueError('Alineacion fuera de rango')
                    datos.extend(bytes((-len(datos))%n))
                elif orden=='rellenar_hasta':
                    n=valor(argumento)
                    if n<len(datos) or n>1048576:raise ValueError('Relleno fuera de rango')
                    datos.extend(bytes(n-len(datos)))
                elif orden in ('guardar32','recuperar32','copiar32','cargar_pila32','devolver',
                               'comparar32','sumar32','decrementar32','direccion_atras'):
                    if modo!=32:raise ValueError('Esta instruccion requiere modo 32')
                    cantidad = 0 if orden in ('devolver','direccion_atras') else (2 if orden in ('copiar32','cargar_pila32','comparar32','sumar32') else 1)
                    if (0 if not argumento else len(args))!=cantidad:
                        raise ValueError('Cantidad de argumentos incorrecta')
                    def registro32(r):
                        if not r.startswith('e'):raise ValueError('Se requiere un registro de 32 bits')
                        return REGISTROS.index(r[1:])
                    if orden=='devolver':datos.append(0xc3)
                    elif orden=='direccion_atras':datos.append(0xfd)
                    elif orden=='decrementar32':datos.append(0x48+registro32(argumento))
                    elif orden in ('guardar32','recuperar32'):
                        datos.append((0x50 if orden=='guardar32' else 0x58)+registro32(argumento))
                    elif orden in ('copiar32','comparar32','sumar32'):
                        opcode={'copiar32':0x89,'comparar32':0x39,'sumar32':0x01}[orden]
                        datos.extend((opcode,0xc0+8*registro32(args[1])+registro32(args[0])))
                    else:
                        n=valor(args[1])
                        if n<0 or n>127:raise ValueError('Desplazamiento de pila fuera de rango')
                        datos.extend((0x8b,0x44+8*registro32(args[0]),0x24,n))
                elif orden in ('comparar_inmediato32','comparar_inmediato16','sumar_inmediato16'):
                    bits=32 if orden.endswith('32') else 16
                    r=args[0]
                    indice=REGISTROS.index(r[1:] if bits==32 and r.startswith('e') else r)
                    if bits==32 and not r.startswith('e'):raise ValueError('Se requiere registro de 32 bits')
                    if bits!=modo:datos.append(0x66)
                    datos.extend((0x81,(0xc0 if orden.startswith('sumar') else 0xf8)+indice))
                    emitir(valor(args[1]),bits//8)
                elif orden in ('guardar16','recuperar16'):
                    if modo!=16:raise ValueError('Se requiere modo 16')
                    datos.append((0x50 if orden=='guardar16' else 0x58)+REGISTROS.index(argumento))
                elif orden=='escribir_memoria16':
                    # Direccion absoluta DS:desplazamiento, valor de un registro de 16 bits.
                    if modo!=16:raise ValueError('Se requiere modo 16')
                    datos.extend((0x89,0x06+8*REGISTROS.index(args[1])))
                    emitir(valor(args[0]),2)
                elif orden=='cero32':
                    if modo!=32 or not argumento.startswith('e'): raise ValueError('Se requiere registro y modo de 32 bits')
                    indice=REGISTROS.index(argumento[1:]); datos.extend((0x31,0xc0+9*indice))
                elif orden in ('guardar_segmento32','recuperar_segmento32'):
                    if modo!=32: raise ValueError('Esta instruccion requiere modo 32')
                    guardar={'ds':b'\x1e','es':b'\x06','fs':b'\x0f\xa0','gs':b'\x0f\xa8'}
                    recuperar={'ds':b'\x1f','es':b'\x07','fs':b'\x0f\xa1','gs':b'\x0f\xa9'}
                    if argumento not in guardar: raise ValueError('Segmento no admitido')
                    datos.extend((guardar if orden=='guardar_segmento32' else recuperar)[argumento])
                elif orden in ('guardar_banderas32','recuperar_banderas32','apilar_inmediato32',
                               'leer_memoria32','escribir_memoria32'):
                    if modo!=32: raise ValueError('Esta instruccion requiere modo 32')
                    cantidad=2 if orden in ('leer_memoria32','escribir_memoria32') else (1 if orden=='apilar_inmediato32' else 0)
                    if (len(args) if argumento else 0)!=cantidad: raise ValueError('Cantidad de argumentos incorrecta')
                    if orden in ('guardar_banderas32','recuperar_banderas32'):
                        datos.append(0x9c if orden=='guardar_banderas32' else 0x9d)
                    elif orden=='apilar_inmediato32': datos.append(0x68); emitir(valor(argumento),4)
                    else:
                        r=args[0] if orden=='leer_memoria32' else args[1]
                        if not r.startswith('e'): raise ValueError('Se requiere registro de 32 bits')
                        indice=REGISTROS.index(r[1:])
                        datos.extend((0x8b if orden=='leer_memoria32' else 0x89,0x05+8*indice))
                        emitir(valor(args[1] if orden=='leer_memoria32' else args[0]),4)
                elif texto in FIJAS:datos.extend(FIJAS[texto])
                elif orden in ('cargar16','cargar32'):
                    registro=args[0]; bits=16 if orden=='cargar16' else 32
                    indice=REGISTROS.index(registro[1:] if bits==32 else registro)
                    if bits!=modo:datos.append(0x66)
                    datos.append(0xb8+indice);emitir(valor(args[1]),bits//8)
                elif orden=='cargar8':
                    indice=['al','cl','dl','bl','ah','ch','dh','bh'].index(args[0])
                    datos.append(0xb0+indice);emitir(valor(args[1]),1)
                elif orden in ('interrupcion','entrada8','salida8','unir8'):
                    datos.append({'interrupcion':0xcd,'entrada8':0xe4,'salida8':0xe6,'unir8':0x0c}[orden]);emitir(valor(argumento),1)
                elif orden=='unir32':
                    if modo!=32:datos.append(0x66)
                    datos.append(0x0d);emitir(valor(argumento),4)
                elif orden=='cargar_gdt':
                    datos.extend(b'\x0f\x01\x16' if modo==16 else b'\x0f\x01\x15')
                    emitir(valor(argumento),modo//8)
                elif orden=='saltar_lejos':
                    datos.append(0xea);emitir(valor(args[1]),modo//8);emitir(valor(args[0]),2)
                elif orden in ('saltar','si_error','si_igual','si_distinto','si_menor_igual_sin_signo'):
                    datos.extend({'saltar':b'\xe9','si_error':b'\x0f\x82',
                                  'si_igual':b'\x0f\x84','si_distinto':b'\x0f\x85',
                                  'si_menor_igual_sin_signo':b'\x0f\x86'}[orden])
                    distancia=valor(argumento)-(origen+len(datos)+modo//8) if pasada else 0
                    emitir(distancia,modo//8,True)
                else:raise ValueError('Instruccion no admitida: '+texto)
            except (ValueError,OverflowError,IndexError,SyntaxError) as e:
                raise ValueError(f'Linea {linea}: {e}') from None
    return bytes(datos)

if __name__=='__main__':
    p=argparse.ArgumentParser(description='NEXTRY ASM 0.1: fuente NXA a binario x86')
    p.add_argument('fuente',type=Path);p.add_argument('-o',required=True,type=Path)
    p.add_argument('--simbolos',type=Path)
    p.add_argument('--permitir-externos',action='store_true')
    a=p.parse_args()
    try:
        simbolos=json.loads(a.simbolos.read_text()) if a.simbolos else None
        binario=ensamblar(a.fuente.read_text(encoding='utf-8-sig'),simbolos,a.permitir_externos)
        a.o.parent.mkdir(parents=True,exist_ok=True);a.o.write_bytes(binario)
    except ValueError as e:p.exit(1,str(e)+'\n')
    print(f'NEXTRY ASM: {len(binario)} bytes generados directamente.')
