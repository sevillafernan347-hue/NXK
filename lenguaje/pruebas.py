# GUIA DE LECTURA NXK - lenguaje/pruebas.py
# Pruebas del compilador: comprueban programas validos y errores de sintaxis, tipos y limites.
# Contexto y orden de estudio: ver LEER-CODIGO.md en la carpeta principal.
"""Pruebas de sintaxis, tipos, generacion segura y limites del lenguaje."""
import unittest
import sys
from pathlib import Path
sys.path.insert(0, str(Path(__file__).resolve().parent))
from compilador import compilar, ErrorLenguaje


class Lenguaje(unittest.TestCase):
    def error(self, texto, mensaje):
        with self.assertRaisesRegex(ErrorLenguaje, mensaje):
            compilar(texto)

    def test_bloques_tipos_y_operadores(self):
        c = compilar('''funcion inicio
variable a = 2
constante b = 3
repetir b veces
a = a + 1
fin
si a == 5 y no falso
numero a
sino
detener
fin
mientras a > 0
a = a - 1
fin
afirmar 7 / 2 == 3
afirmar 7 % 2 == 1
afirmar (1 << 33) == 2
fin''')
        self.assertIn('for (uint32_t', c)
        self.assertIn('while (', c)
        self.assertIn('nxk_es_dividir', c)
        self.assertIn('nxk_es_resto', c)
        self.assertIn('& UINT32_C(31)', c)

    def test_acentos_comentarios_y_texto(self):
        c = compilar('''# Comentario
función inicio
variable señal = 1
número señal
mostrar "Hola, español"
fin''')
        self.assertIn('nxk_es_numero', c)
        self.assertIn(r'\xc3\xb1', c)
        self.assertNotIn('señal', c)  # Identificador convertido en simbolo C estable.

    def test_texto_no_inyecta_codigo_c(self):
        c = compilar('funcion inicio\nmostrar "\\\"; detener(); //"\nfin')
        self.assertIn(r'\x22\x3b', c)
        self.assertNotIn('detener(); //', c)

    def test_ambito_de_bloque(self):
        self.error('funcion inicio\nsi verdadero\nvariable local = 1\nfin\nnumero local\nfin', 'no existe')

    def test_constantes(self):
        self.error('funcion inicio\nconstante a = 1\na = 2\nfin', 'constante')

    def test_asignacion_con_otro_tipo(self):
        self.error('funcion inicio\nvariable a = 1\na = "texto"\nfin', 'entero')

    def test_condicion_debe_ser_logica(self):
        self.error('funcion inicio\nsi 1\nfin\nfin', 'logico')

    def test_mostrar_exige_texto(self):
        self.error('funcion inicio\nmostrar 3\nfin', 'texto')

    def test_funciones_adelantadas(self):
        self.assertIn('nxk_es_inicio', compilar('funcion inicio\nllamar otra\nfin\nfuncion otra\nvolver\nfin'))

    def test_funcion_desconocida(self):
        self.error('funcion inicio\nllamar otra\nfin', 'no existe')

    def test_recursion_indirecta(self):
        self.error('funcion inicio\nllamar otra\nfin\nfuncion otra\nllamar inicio\nfin', 'recursivas')

    def test_limites_y_divisor_cero(self):
        self.error('funcion inicio\nnumero 4294967296\nfin', 'supera')
        self.error('funcion inicio\nnumero 7 / 0\nfin', 'cero')

    def test_nulo_y_escapes(self):
        self.error('funcion inicio\nmostrar "\\u0000"\nfin', 'nulo')
        self.error('funcion inicio\nmostrar "\\q"\nfin', 'escape')

    def test_errores_de_estructura(self):
        self.error('funcion inicio\nmostrar "hola"', 'fin')
        self.error('funcion otra\nfin', 'inicio')
        self.error('funcion inicio\nfin\nfuncion inicio\nfin', 'dos veces')

    def test_sin_sombreado(self):
        self.error('funcion inicio\nvariable a = 1\nsi verdadero\nvariable a = 2\nfin\nfin', 'declarado')

    def test_puertos(self):
        c = compilar('funcion inicio\nvariable estado = 0\nleer_puerto estado, 0x3fd\nescribir_puerto 0xe9, 65\nfin')
        self.assertIn('nxk_es_leer_puerto', c)
        self.assertIn('nxk_es_escribir_puerto', c)

    def test_error_tiene_linea(self):
        try:
            compilar('funcion inicio\nnumero desconocida\nfin')
        except ErrorLenguaje as e:
            self.assertEqual(e.linea, 2)
        else:
            self.fail('Se esperaba un diagnostico')


if __name__ == '__main__':
    unittest.main(verbosity=2)
