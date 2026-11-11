"""Sintaxis, tipos y ejemplos reales de buffers protegidos NEXTRY."""
import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from compilador import compilar, ErrorLenguaje, INTRINSECOS_BUFFER


class BuffersNextry(unittest.TestCase):
    def fuente(self, orden):
        return 'funcion inicio\n' + orden + '\nfin'

    def error(self, orden, patron):
        with self.assertRaisesRegex(ErrorLenguaje, patron):
            compilar(self.fuente(orden))

    def test_ejemplos_reales(self):
        for nombre in ('buffer_archivos', 'memoria_mensajes'):
            fuente=(Path(__file__).parent/'ejemplos'/(nombre+'.nxf')).read_text(encoding='utf-8')
            codigo=compilar(fuente)
            self.assertIn('nxk_es_finalizar(UINT32_C(42))', codigo)
            self.assertIn('nxk_es_reservar_pagina()', codigo)

    def test_lectura_y_guardado_orden_de_argumentos(self):
        codigo=compilar(self.fuente('numero leer_archivo_en("a", 1, 2, 3)\n'
                                   'numero guardar_archivo_desde("b", 4, 5)'))
        self.assertIn('nxk_es_leer_archivo_en("\\x61", UINT32_C(1), UINT32_C(2), UINT32_C(3))', codigo)
        self.assertIn('nxk_es_guardar_archivo_desde("\\x62", UINT32_C(4), UINT32_C(5))', codigo)

    def test_mostrar_recibir_esperar_orden_de_argumentos(self):
        codigo=compilar(self.fuente('numero mostrar_memoria(1, 2)\n'
                                   'numero recibir_en_memoria(3, 4)\n'
                                   'numero esperar_en_memoria(5, 6, 7)'))
        for nombre, valores in (('mostrar_memoria', (1,2)), ('recibir_en_memoria', (3,4)),
                                ('esperar_en_memoria', (5,6,7))):
            self.assertIn('nxk_es_'+nombre+'('+', '.join('UINT32_C('+str(v)+')' for v in valores)+')', codigo)

    def test_tipos_de_todos_los_argumentos(self):
        for nombre, tipos in INTRINSECOS_BUFFER.items():
            valores=['"a"' if t=='texto' else '1' for t in tipos]
            for i, tipo in enumerate(tipos):
                with self.subTest(nombre=nombre, indice=i):
                    invalidos=('1', 'verdadero') if tipo=='texto' else ('"a"', 'verdadero')
                    for invalido in invalidos:
                        otros=valores.copy(); otros[i]=invalido
                        self.error('numero '+nombre+'('+', '.join(otros)+')', tipo)

    def test_aridad_y_parentesis(self):
        for nombre, tipos in INTRINSECOS_BUFFER.items():
            valores=['"a"' if t=='texto' else '1' for t in tipos]
            with self.subTest(nombre=nombre):
                self.error('numero '+nombre, r'\(')
                self.error('numero '+nombre+'()', 'expresion')
                self.error('numero '+nombre+'('+', '.join(valores[:-1])+')', ',')
                self.error('numero '+nombre+'('+', '.join(valores+['1'])+')', r'\)')

    def test_resultado_es_entero(self):
        for nombre, tipos in INTRINSECOS_BUFFER.items():
            expresion=nombre+'('+', '.join('"a"' if t=='texto' else '1' for t in tipos)+')'
            self.assertIn('nxk_es_'+nombre+'(', compilar(self.fuente('entero estado = '+expresion)))
            self.error('logico estado = '+expresion, 'logico')
            self.error('texto estado = '+expresion, 'texto')

    def test_palabras_reservadas_y_solo_expresion(self):
        for nombre, tipos in INTRINSECOS_BUFFER.items():
            self.error('entero '+nombre+' = 1', 'NOMBRE')
            llamada=nombre+'('+', '.join('"a"' if t=='texto' else '1' for t in tipos)+')'
            self.error(llamada, 'Orden no reconocida')

    def test_expresiones_y_bloques(self):
        codigo=compilar(self.fuente('entero pagina = reservar_pagina()\n'
            'texto nombre = "buf"\nsi pagina != 0\n'
            'afirmar guardar_archivo_desde(nombre, pagina + 4, 2 + 2) == 0\n'
            'entero leidos = leer_archivo_en(nombre, pagina, 4, 0)\n'
            'afirmar leidos == 4\n'
            'afirmar recibir_en_memoria(pagina, 0) != 0xffffffff\nfin'))
        self.assertIn('nxk_es_guardar_archivo_desde(', codigo)
        self.assertIn('nxk_es_recibir_en_memoria(', codigo)


if __name__ == '__main__':
    unittest.main(verbosity=2)
