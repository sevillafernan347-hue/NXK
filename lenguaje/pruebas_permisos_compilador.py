"""Sintaxis y tipos del control de permisos de procesos NEXTRY."""
import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from compilador import compilar, ErrorLenguaje


class PermisosNextry(unittest.TestCase):
    def fuente(self, orden):
        return 'funcion inicio\n' + orden + '\nfin'

    def error(self, orden, patron):
        with self.assertRaisesRegex(ErrorLenguaje, patron):
            compilar(self.fuente(orden))

    def test_ejemplo_completo(self):
        fuente=(Path(__file__).parent/'ejemplos/proceso_permisos.nxf').read_text(encoding='utf-8')
        codigo=compilar(fuente)
        self.assertIn('nxk_es_restringir_permisos(', codigo)
        self.assertIn('nxk_es_permisos_actuales()', codigo)
        self.assertIn('nxk_es_finalizar(UINT32_C(42))', codigo)

    def test_restringir_expresiones_enteras(self):
        for orden in ('restringir_permisos 0', 'restringir_permisos 15',
                      'entero mascara = 7\nrestringir_permisos mascara',
                      'restringir_permisos permisos_actuales() y_bits 3'):
            with self.subTest(orden=orden):
                self.assertIn('nxk_es_restringir_permisos(', compilar(self.fuente(orden)))

    def test_restringir_exige_entero(self):
        for valor in ('verdadero', '"3"'):
            self.error('restringir_permisos '+valor, 'entero')

    def test_consulta_tiene_tipo_entero(self):
        self.assertIn('nxk_es_permisos_actuales()', compilar(self.fuente(
            'entero actuales = permisos_actuales()\nnumero actuales\nafirmar actuales == 15')))
        self.error('logico actuales = permisos_actuales()', 'logico')
        self.error('texto actuales = permisos_actuales()', 'texto')

    def test_consulta_exige_parentesis_vacios(self):
        self.error('numero permisos_actuales', r'\(')
        self.error('numero permisos_actuales(1)', r'\)')
        self.error('numero permisos_actuales(1, 2)', r'\)')

    def test_forma_de_la_orden(self):
        self.error('restringir_permisos', 'expresion')
        self.error('restringir_permisos 3, 1', 'NL')
        self.error('numero restringir_permisos', 'expresion')
        self.error('permisos_actuales()', 'Orden no reconocida')

    def test_palabras_reservadas(self):
        for nombre in ('restringir_permisos', 'permisos_actuales'):
            self.error('entero '+nombre+' = 3', 'NOMBRE')

    def test_en_bloques_y_funciones(self):
        fuente=self.fuente('mientras verdadero\nrestringir_permisos 3\nromper\nfin\nllamar reducir')
        fuente+='\nfuncion reducir\nrestringir_permisos permisos_actuales() y_bits 1\nfin'
        self.assertEqual(compilar(fuente).count('nxk_es_restringir_permisos('), 2)


if __name__ == '__main__':
    unittest.main(verbosity=2)
