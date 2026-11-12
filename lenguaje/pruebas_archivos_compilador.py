"""Sintaxis y tipos de la reduccion de archivos permitidos por proceso."""
import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from compilador import compilar, ErrorLenguaje


class ArchivosNextry(unittest.TestCase):
    def fuente(self, orden):
        return 'funcion inicio\n' + orden + '\nfin'

    def error(self, orden, patron):
        with self.assertRaisesRegex(ErrorLenguaje, patron):
            compilar(self.fuente(orden))

    def test_lista_vacia_revoca_todo(self):
        self.assertIn('nxk_es_limitar_archivos(UINT32_C(0), 0, 0, 0, 0);',
                      compilar(self.fuente('limitar_archivos')))

    def test_cantidades_y_orden(self):
        for cantidad in range(1, 5):
            nombres=['"'+chr(97+i)+'"' for i in range(cantidad)]
            generado=compilar(self.fuente('limitar_archivos '+', '.join(nombres)))
            argumentos=['UINT32_C('+str(cantidad)+')']
            argumentos+=['"\\x'+format(97+i,'02x')+'"' for i in range(cantidad)]
            argumentos+=['0']*(4-cantidad)
            self.assertIn('nxk_es_limitar_archivos('+', '.join(argumentos)+');', generado)

    def test_limite_de_cuatro(self):
        self.error('limitar_archivos "a", "b", "c", "d", "e"', 'maximo cuatro')

    def test_nombres_exigen_texto(self):
        for orden in ('limitar_archivos 1', 'limitar_archivos verdadero',
                      'limitar_archivos "a", 2', 'limitar_archivos "a", falso'):
            self.error(orden, 'texto')

    def test_comas_y_orden_completa(self):
        self.error('limitar_archivos "a",', 'expresion')
        self.error('limitar_archivos , "a"', 'expresion')
        self.error('limitar_archivos "a" "b"', 'NL')
        self.error('limitar_archivos()', 'expresion')

    def test_solo_orden_y_palabra_reservada(self):
        self.error('numero limitar_archivos', 'expresion')
        self.error('entero limitar_archivos = 0', 'NOMBRE')

    def test_variables_y_bloques(self):
        fuente=self.fuente('texto nombre = "propio"\nsi verdadero\n'
            'limitar_archivos nombre, "copia"\nfin\nllamar reducir')
        fuente+='\nfuncion reducir\nlimitar_archivos\nfin'
        codigo=compilar(fuente)
        self.assertEqual(codigo.count('nxk_es_limitar_archivos('), 2)
        self.assertIn('UINT32_C(2),', codigo)

    def test_ejemplo_real(self):
        fuente=(Path(__file__).parent/'ejemplos/proceso_archivos.nxf').read_text(encoding='utf-8')
        codigo=compilar(fuente)
        self.assertEqual(codigo.count('nxk_es_limitar_archivos('), 3)
        self.assertIn('nxk_es_finalizar(UINT32_C(42))', codigo)
        self.assertIn('nxk_es_guardar_archivo_desde(', codigo)


if __name__ == '__main__':
    unittest.main(verbosity=2)
