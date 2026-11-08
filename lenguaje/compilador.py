# GUIA DE LECTURA NXK - lenguaje/compilador.py
# Compilador NEXTRY: separa palabras, analiza estructura y tipos, y produce C sin sistema operativo anfitrion.
# Contexto y orden de estudio: ver LEER-CODIGO.md en la carpeta principal.
"""Compilador de NXK Espanol 0.1: analisis propio y salida C freestanding.

No ejecuta el programa fuente. No usa eval, exec ni el parser de Python.
"""
import argparse
import json
import re
import sys
import unicodedata
from dataclasses import dataclass
from pathlib import Path


class ErrorLenguaje(Exception):
    def __init__(self, mensaje, linea=1, columna=1):
        self.linea, self.columna = linea, columna
        super().__init__(mensaje)


@dataclass
class Token:
    tipo: str
    texto: str
    linea: int
    columna: int


PALABRAS = set('funcion fin variable constante si sino mientras repetir veces '
               'mostrar numero llamar volver detener ceder dormir afirmar leer_puerto '
               'escribir_puerto verdadero falso y o no y_bits o_bits xor_bits '
               'memoria_usada proceso_actual enviar_mensaje recibir_mensaje '
               'enviar_esperando esperar_proceso finalizar restringir_permisos permisos_actuales '
               'romper continuar entero logico texto guardar_archivo mostrar_archivo borrar_archivo agregar_archivo renombrar_archivo copiar_archivo'.split())
ALIAS = {'función': 'funcion', 'número': 'numero'}
PATRON = re.compile(
    r'(?P<ESPACIO>[^\S\n]+)|(?P<COMENTARIO>\#[^\n]*)|(?P<NL>\n)|'
    r'(?P<TEXTO>"(?:[^"\\\n]|\\[^\n])*")|'
    r'(?P<ENTERO>0[xX][0-9a-fA-F]+|[0-9]+)|'
    r'(?P<NOMBRE>[^\W\d]\w*)|'
    r'(?P<OP>==|!=|<=|>=|<<|>>|[=+*/%<>()\-,])', re.UNICODE)


def tokens(fuente):
    # Primera etapa: convierte texto en piezas (nombres, numeros y operadores).
    # Guarda linea y columna para explicar al usuario donde esta un error.
    fuente = unicodedata.normalize('NFC', fuente).replace('\r\n', '\n').replace('\r', '\n')
    salida, pos, linea, columna = [], 0, 1, 1
    while pos < len(fuente):
        m = PATRON.match(fuente, pos)
        if not m:
            raise ErrorLenguaje('Caracter no reconocido o texto sin cerrar.', linea, columna)
        tipo, texto = m.lastgroup, m.group()
        if tipo not in ('ESPACIO', 'COMENTARIO'):
            if tipo == 'NOMBRE':
                texto = ALIAS.get(texto, texto)
                if texto in PALABRAS:
                    tipo = texto
            salida.append(Token(tipo, texto, linea, columna))
        if tipo == 'NL':
            linea, columna = linea + 1, 1
        else:
            columna += len(m.group())
        pos = m.end()
    salida.append(Token('NL', '\n', linea, columna))
    salida.append(Token('FINAL', '', linea + 1, 1))
    return salida


PRECEDENCIA = {'o': 1, 'y': 2, '==': 3, '!=': 3,
               '<': 4, '>': 4, '<=': 4, '>=': 4,
               'o_bits': 5, 'xor_bits': 6, 'y_bits': 7,
               '<<': 8, '>>': 8, '+': 9, '-': 9,
               '*': 10, '/': 10, '%': 10}


class Analizador:
    def __init__(self, fuente):
        self.ts, self.i, self.profundidad = tokens(fuente), 0, 0

    @property
    def actual(self):
        return self.ts[self.i]

    def error(self, mensaje, token=None):
        t = token or self.actual
        raise ErrorLenguaje(mensaje, t.linea, t.columna)

    def tomar(self, tipo=None):
        t = self.actual
        if tipo is not None and t.tipo != tipo and t.texto != tipo:
            self.error(f'Se esperaba «{tipo}»; se encontro «{t.texto or "final del archivo"}».')
        self.i += 1
        return t

    def linea(self):
        self.tomar('NL')
        while self.actual.tipo == 'NL':
            self.tomar()

    def expresion(self, minimo=0):
        self.profundidad += 1
        if self.profundidad > 100:
            self.error('Expresion demasiado anidada; dividela en variables.')
        t = self.tomar()
        if t.tipo == 'ENTERO':
            valor = int(t.texto, 16 if t.texto.lower().startswith('0x') else 10)
            if valor > 0xffffffff:
                self.error('El entero supera 4294967295.', t)
            izq = ('entero', t, valor)
        elif t.tipo == 'TEXTO':
            try:
                valor = json.loads(t.texto)
                valor.encode('utf-8')
                if '\x00' in valor:
                    self.error('Los textos no pueden contener el caracter nulo.', t)
            except (ValueError, UnicodeError):
                self.error('Texto o secuencia de escape incorrecta.', t)
            izq = ('texto', t, valor)
        elif t.tipo in ('verdadero', 'falso'):
            izq = ('logico', t, t.tipo == 'verdadero')
        elif t.tipo == 'NOMBRE':
            izq = ('nombre', t, t.texto)
        elif t.tipo in ('memoria_usada', 'proceso_actual', 'permisos_actuales'):
            self.tomar('('); self.tomar(')')
            izq = (t.tipo, t)
        elif t.texto == '(':
            izq = self.expresion(); self.tomar(')')
        elif t.tipo == 'no' or t.texto == '-':
            izq = ('unario', t, self.expresion(11))
        else:
            self.error('Se esperaba un numero, texto, variable o expresion.', t)
        while self.actual.texto in PRECEDENCIA and PRECEDENCIA[self.actual.texto] >= minimo:
            op = self.tomar()
            der = self.expresion(PRECEDENCIA[op.texto] + 1)
            izq = ('binario', op, izq, der)
        self.profundidad -= 1
        return izq

    def bloque(self, profundidad=0):
        if profundidad > 40:
            self.error('Demasiados bloques anidados.')
        salida = []
        while self.actual.tipo not in ('fin', 'sino', 'FINAL'):
            t = self.tomar(); k = t.tipo
            if k in ('variable', 'constante', 'entero', 'logico', 'texto'):
                declarado = k if k in ('entero', 'logico', 'texto') else None
                clase = 'variable' if declarado else k
                # Tambien permite constante entero limite = 3.
                if k in ('variable', 'constante') and self.actual.tipo in ('entero','logico','texto'):
                    declarado = self.tomar().tipo
                n = self.tomar('NOMBRE'); self.tomar('=')
                salida.append((clase, t, n, self.expresion(), declarado)); self.linea()
            elif k == 'NOMBRE':
                self.tomar('=')
                salida.append(('asignar', t, self.expresion())); self.linea()
            elif k in ('guardar_archivo','agregar_archivo','renombrar_archivo','copiar_archivo'):
                nombre=self.expresion(); self.tomar(',')
                salida.append((k,t,nombre,self.expresion())); self.linea()
            elif k in ('mostrar_archivo','borrar_archivo'):
                salida.append((k,t,self.expresion())); self.linea()
            elif k in ('mostrar', 'numero', 'dormir', 'finalizar', 'restringir_permisos', 'afirmar'):
                salida.append((k, t, self.expresion())); self.linea()
            elif k in ('volver', 'detener', 'ceder', 'romper', 'continuar'):
                salida.append((k, t)); self.linea()
            elif k == 'llamar':
                salida.append((k, t, self.tomar('NOMBRE'))); self.linea()
            elif k == 'leer_puerto':
                n = self.tomar('NOMBRE'); self.tomar(',')
                salida.append((k, t, n, self.expresion())); self.linea()
            elif k == 'escribir_puerto':
                puerto = self.expresion(); self.tomar(',')
                salida.append((k, t, puerto, self.expresion())); self.linea()
            elif k == 'enviar_mensaje':
                destino = self.expresion(); self.tomar(',')
                salida.append((k, t, destino, self.expresion())); self.linea()
            elif k == 'recibir_mensaje':
                # Tres variables de salida y un plazo; nunca expresiones/punteros arbitrarios.
                valor = self.tomar('NOMBRE'); self.tomar(',')
                origen = self.tomar('NOMBRE'); self.tomar(',')
                resultado = self.tomar('NOMBRE'); self.tomar(',')
                salida.append((k, t, valor, origen, resultado, self.expresion())); self.linea()
            elif k == 'enviar_esperando':
                destino = self.expresion(); self.tomar(',')
                valor = self.expresion(); self.tomar(',')
                resultado = self.tomar('NOMBRE'); self.tomar(',')
                salida.append((k, t, destino, valor, resultado, self.expresion())); self.linea()
            elif k == 'esperar_proceso':
                destino = self.expresion(); self.tomar(',')
                estado = self.tomar('NOMBRE'); self.tomar(',')
                codigo = self.tomar('NOMBRE'); self.tomar(',')
                resultado = self.tomar('NOMBRE'); self.tomar(',')
                salida.append((k, t, destino, estado, codigo, resultado, self.expresion())); self.linea()
            elif k in ('si', 'mientras', 'repetir'):
                condicion = self.expresion()
                if k == 'repetir': self.tomar('veces')
                self.linea(); cuerpo = self.bloque(profundidad + 1); otro = []
                if k == 'si' and self.actual.tipo == 'sino':
                    self.tomar(); self.linea(); otro = self.bloque(profundidad + 1)
                self.tomar('fin'); self.linea()
                salida.append((k, t, condicion, cuerpo, otro))
            else:
                self.error('Orden no reconocida. Revisa las palabras clave.', t)
        return salida

    def programa(self):
        funciones = []
        while self.actual.tipo == 'NL': self.tomar()
        while self.actual.tipo != 'FINAL':
            self.tomar('funcion'); n = self.tomar('NOMBRE'); self.linea()
            cuerpo = self.bloque()
            self.tomar('fin'); self.linea()
            funciones.append((n, cuerpo))
        return funciones


def simbolo(nombre):
    return 'n_' + nombre.encode('utf-8').hex()


class Generador:
    def __init__(self, funciones):
        self.funciones = funciones
        self.nombres = {}
        self.ambitos = []
        self.llamadas = {}
        self.funcion = ''
        self.serie = 0
        self.bucles = 0

    def error(self, t, mensaje):
        raise ErrorLenguaje(mensaje, t.linea, t.columna)

    def buscar(self, t):
        for ambito in reversed(self.ambitos):
            if t.texto in ambito: return ambito[t.texto]
        self.error(t, f'La variable «{t.texto}» no existe en este bloque.')

    def tipo(self, t, obtenido, esperado):
        if obtenido != esperado:
            self.error(t, f'Se requiere un valor {esperado}; se recibio {obtenido}.')

    def salidas_enteras(self, destinos):
        # Nunca entregar punteros a logicos (_Bool), constantes o alias:
        # los servicios publican palabras completas de 32 bits.
        nombres = []
        for destino in destinos:
            tipo, nombre, constante = self.buscar(destino)
            self.tipo(destino, tipo, 'entero')
            if constante: self.error(destino, 'El destino debe ser una variable modificable.')
            if nombre in nombres: self.error(destino, 'Los destinos deben ser variables distintas.')
            nombres.append(nombre)
        return nombres

    def expresion(self, e):
        k, t = e[:2]
        if k == 'entero': return 'entero', f'UINT32_C({e[2]})'
        if k == 'texto':
            return 'texto', '"' + ''.join(f'\\x{b:02x}' for b in e[2].encode('utf-8')) + '"'
        if k == 'logico': return 'logico', '1' if e[2] else '0'
        if k == 'nombre':
            tipo, nombre, _ = self.buscar(t); return tipo, nombre
        if k in ('memoria_usada', 'proceso_actual', 'permisos_actuales'): return 'entero', f'nxk_es_{k}()'
        if k == 'unario':
            tipo, a = self.expresion(e[2])
            self.tipo(t, tipo, 'logico' if t.tipo == 'no' else 'entero')
            return tipo, f'(!({a}))' if t.tipo == 'no' else f'(UINT32_C(0) - ({a}))'
        ta, a = self.expresion(e[2]); tb, b = self.expresion(e[3]); op = t.texto
        if op in ('y', 'o'):
            self.tipo(t, ta, 'logico'); self.tipo(t, tb, 'logico')
            return 'logico', f'(({a}) {"&&" if op == "y" else "||"} ({b}))'
        # Comparaciones solo entre enteros/logicos; texto no tiene igualdad implicita de punteros.
        if op in ('==', '!=') and ta == tb == 'logico':
            return 'logico', f'(({a}) {op} ({b}))'
        self.tipo(t, ta, 'entero'); self.tipo(t, tb, 'entero')
        if op in ('/', '%'):
            if e[3][0] == 'entero' and e[3][2] == 0:
                self.error(t, 'No se puede dividir entre cero.')
            f = 'dividir' if op == '/' else 'resto'
            return 'entero', f'nxk_es_{f}({a}, {b})'
        if op in ('<<', '>>'):
            return 'entero', f'(({a}) {op} (({b}) & UINT32_C(31)))'
        operador = {'y_bits': '&', 'o_bits': '|', 'xor_bits': '^'}.get(op, op)
        tipo = 'logico' if op in ('==', '!=', '<', '>', '<=', '>=') else 'entero'
        return tipo, f'(({a}) {operador} ({b}))'

    def bloque(self, ordenes, nivel=1):
        self.ambitos.append({}); salida = []
        def poner(texto): salida.append('    ' * nivel + texto)
        for s in ordenes:
            k, t = s[:2]
            if k in ('variable', 'constante'):
                n = s[2]
                if any(n.texto in a for a in self.ambitos):
                    self.error(n, f'«{n.texto}» ya esta declarado; usa otro nombre.')
                tipo, valor = self.expresion(s[3]); self.serie += 1
                if len(s)>4 and s[4]: self.tipo(t,tipo,s[4])
                nombre = simbolo(n.texto) + '_' + str(self.serie)
                self.ambitos[-1][n.texto] = (tipo, nombre, k == 'constante')
                c_tipo = {'entero': 'uint32_t', 'logico': '_Bool', 'texto': 'const char *'}[tipo]
                # Se controla inmutabilidad aqui; no depende de advertencias del compilador C.
                poner(f'{c_tipo} {nombre} = {valor};')
                poner(f'(void){nombre};')
            elif k == 'asignar':
                tipo, nombre, constante = self.buscar(t)
                if constante: self.error(t, 'No se puede cambiar una constante.')
                nuevo, valor = self.expresion(s[2]); self.tipo(t, nuevo, tipo)
                poner(f'{nombre} = {valor};')
            elif k in ('guardar_archivo','agregar_archivo','renombrar_archivo','copiar_archivo','mostrar_archivo','borrar_archivo'):
                tipo,nombre=self.expresion(s[2]); self.tipo(t,tipo,'texto')
                if k in ('guardar_archivo','agregar_archivo','renombrar_archivo','copiar_archivo'):
                    tipo,datos=self.expresion(s[3]); self.tipo(t,tipo,'texto')
                    poner(f'nxk_es_{k}({nombre}, {datos});')
                else: poner(f'nxk_es_{k}({nombre});')
            elif k in ('mostrar', 'numero', 'dormir', 'finalizar', 'restringir_permisos', 'afirmar'):
                tipo, valor = self.expresion(s[2])
                esperado = {'mostrar': 'texto', 'numero': 'entero', 'dormir': 'entero', 'finalizar': 'entero', 'restringir_permisos': 'entero', 'afirmar': 'logico'}[k]
                self.tipo(t, tipo, esperado)
                if k == 'afirmar': poner(f'if (!({valor})) nxk_es_fallo(UINT32_C({t.linea}));')
                else: poner(f'nxk_es_{k}({valor});')
            elif k == 'llamar':
                n = s[2]
                if n.texto not in self.nombres: self.error(n, f'La funcion «{n.texto}» no existe.')
                self.llamadas[self.funcion].add(n.texto)
                poner(f'{self.nombres[n.texto]}();')
            elif k == 'ceder':
                poner('nxk_es_ceder();')
            elif k in ('volver', 'detener'):
                poner('return;' if k == 'volver' else 'nxk_es_detener();')
            elif k in ('romper', 'continuar'):
                if not self.bucles: self.error(t, 'Esta orden requiere estar dentro de un bucle.')
                poner('break;' if k == 'romper' else 'continue;')
            elif k == 'leer_puerto':
                tipo, nombre, constante = self.buscar(s[2])
                self.tipo(s[2], tipo, 'entero')
                if constante: self.error(s[2], 'El destino de lectura debe ser una variable.')
                pt, puerto = self.expresion(s[3]); self.tipo(t, pt, 'entero')
                poner(f'{nombre} = nxk_es_leer_puerto({puerto});')
            elif k == 'escribir_puerto':
                pt, puerto = self.expresion(s[2]); vt, valor = self.expresion(s[3])
                self.tipo(t, pt, 'entero'); self.tipo(t, vt, 'entero')
                poner(f'nxk_es_escribir_puerto({puerto}, {valor});')
            elif k == 'enviar_mensaje':
                dt, destino = self.expresion(s[2]); vt, valor = self.expresion(s[3])
                self.tipo(t, dt, 'entero'); self.tipo(t, vt, 'entero')
                poner(f'nxk_es_enviar_mensaje({destino}, {valor});')
            elif k == 'recibir_mensaje':
                nombres = []
                for destino in s[2:5]:
                    tipo, nombre, constante = self.buscar(destino)
                    self.tipo(destino, tipo, 'entero')
                    if constante: self.error(destino, 'El destino del mensaje debe ser una variable.')
                    if nombre in nombres: self.error(destino, 'Los destinos del mensaje deben ser variables distintas.')
                    nombres.append(nombre)
                pt, plazo = self.expresion(s[5]); self.tipo(t, pt, 'entero')
                valor, origen, resultado = nombres
                poner(f'{resultado} = nxk_es_recibir_mensaje(&{origen}, &{valor}, {plazo});')
            elif k == 'enviar_esperando':
                dt, destino = self.expresion(s[2]); vt, valor = self.expresion(s[3])
                self.tipo(t, dt, 'entero'); self.tipo(t, vt, 'entero')
                resultado, = self.salidas_enteras((s[4],))
                pt, plazo = self.expresion(s[5]); self.tipo(t, pt, 'entero')
                poner(f'{resultado} = nxk_es_enviar_esperando({destino}, {valor}, {plazo});')
            elif k == 'esperar_proceso':
                dt, destino = self.expresion(s[2]); self.tipo(t, dt, 'entero')
                estado, codigo, resultado = self.salidas_enteras(s[3:6])
                pt, plazo = self.expresion(s[6]); self.tipo(t, pt, 'entero')
                poner(f'{resultado} = nxk_es_esperar_proceso({destino}, &{estado}, &{codigo}, {plazo});')
            else:
                tipo, condicion = self.expresion(s[2])
                self.tipo(t, tipo, 'entero' if k == 'repetir' else 'logico')
                if k == 'repetir':
                    self.serie += 1; n = 'repeticion_' + str(self.serie)
                    poner('{')
                    poner(f'    const uint32_t {n}_limite = {condicion};')
                    poner(f'    for (uint32_t {n} = 0; {n} < {n}_limite; ++{n}) {{')
                    self.bucles += 1
                    salida += self.bloque(s[3], nivel + 2)
                    self.bucles -= 1
                    poner('    }'); poner('}')
                else:
                    poner(f'{"if" if k == "si" else "while"} (({condicion}) != 0) {{')
                    if k == 'mientras': self.bucles += 1
                    salida += self.bloque(s[3], nivel + 1); poner('}')
                    if k == 'mientras': self.bucles -= 1
                    if s[4]:
                        poner('else {'); salida += self.bloque(s[4], nivel + 1); poner('}')
        self.ambitos.pop()
        return salida

    def generar(self):
        for t, _ in self.funciones:
            if t.texto in self.nombres: self.error(t, 'La funcion esta declarada dos veces.')
            self.nombres[t.texto] = 'nxk_es_f_' + simbolo(t.texto)
            self.llamadas[t.texto] = set()
        if 'inicio' not in self.nombres:
            raise ErrorLenguaje('Falta la funcion obligatoria «inicio».')
        salida = ['/* Generado por NXK Espanol 0.1. No editar este archivo. */', '#include "nxk_es.h"']
        salida += [f'void {n}(void);' for n in self.nombres.values()]
        for t, cuerpo in self.funciones:
            self.funcion = t.texto
            salida.append(f'void {self.nombres[t.texto]}(void) {{')
            salida += self.bloque(cuerpo); salida.append('}')
        pendientes, visitados = set(), set()
        def visitar(n):
            if n in pendientes:
                self.error(next(t for t, _ in self.funciones if t.texto == n), 'La version 0.1 no admite llamadas recursivas.')
            if n in visitados: return
            pendientes.add(n)
            for otro in self.llamadas[n]: visitar(otro)
            pendientes.remove(n); visitados.add(n)
        for n in self.nombres: visitar(n)
        salida.append(f'void nxk_es_inicio(void) {{ {self.nombres["inicio"]}(); }}')
        return '\n'.join(salida) + '\n'


def compilar(fuente):
    if len(fuente.encode('utf-8')) > 256 * 1024:
        raise ErrorLenguaje('Archivo demasiado grande: el limite inicial es 256 KiB.')
    return Generador(Analizador(fuente).programa()).generar()


def principal():
    ap = argparse.ArgumentParser(description='Compilador de NXK Espanol 0.1: fuente .nxf hacia C para NXK.')
    ap.add_argument('archivo', type=Path)
    ap.add_argument('-o', '--salida', required=True, type=Path)
    args = ap.parse_args()
    try:
        codigo = compilar(args.archivo.read_text(encoding='utf-8-sig'))
        args.salida.parent.mkdir(parents=True, exist_ok=True)
        args.salida.write_text(codigo, encoding='utf-8')
    except ErrorLenguaje as e:
        print(f'{args.archivo}:{e.linea}:{e.columna}: error: {e}', file=sys.stderr)
        return 1
    except (OSError, UnicodeError) as e:
        print(f'error: No se pudo leer o guardar el archivo: {e}', file=sys.stderr)
        return 1
    except RecursionError:
        print('error: El programa tiene demasiados elementos anidados.', file=sys.stderr)
        return 1
    print(f'NXK Espanol: compilacion correcta -> {args.salida}')
    return 0


if __name__ == '__main__':
    sys.exit(principal())
