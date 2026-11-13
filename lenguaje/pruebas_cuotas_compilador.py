"""Aridad, tipos y composicion de cuotas NEXTRY."""
import sys
import unittest
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parent))
from compilador import compilar,ErrorLenguaje

class Cuotas(unittest.TestCase):
    def compilar(self,orden):
        return compilar('funcion inicio\n'+orden+'\nfin')
    def test_ejemplo(self):
        self.assertIn('nxk_es_limitar_memoria(',compilar((Path(__file__).parent/'ejemplos/cuota_memoria.nxf').read_text()))
    def test_tipos(self):
        for orden in ('limitar_memoria verdadero','limitar_memoria "2"','texto x = cuota_memoria()'):
            with self.subTest(orden=orden),self.assertRaises(ErrorLenguaje):self.compilar(orden)
    def test_aridad(self):
        for orden in ('numero cuota_memoria(1)','numero cuota_memoria','limitar_memoria','limitar_memoria 2, 3'):
            with self.subTest(orden=orden),self.assertRaises(ErrorLenguaje):self.compilar(orden)
    def test_expresiones(self):
        self.assertIn('nxk_es_limitar_memoria(',self.compilar('limitar_memoria cuota_memoria() - 1'))
    def test_reservadas(self):
        for nombre in ('cuota_memoria','limitar_memoria'):
            with self.assertRaises(ErrorLenguaje):self.compilar('entero '+nombre+' = 0')

if __name__=='__main__':unittest.main()
