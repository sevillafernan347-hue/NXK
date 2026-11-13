"""Tipos y listas de destinos de mensajes en NEXTRY."""
import sys
import unittest
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parent))
from compilador import compilar,ErrorLenguaje

class Destinos(unittest.TestCase):
    def fuente(self,orden):return 'funcion inicio\n'+orden+'\nfin'
    def test_cantidades(self):
        for n in range(5):
            c=compilar(self.fuente('limitar_destinos '+', '.join(str(257+i) for i in range(n))))
            self.assertIn('nxk_es_limitar_destinos(UINT32_C('+str(n)+')',c)
    def test_tipos(self):
        for orden in ('limitar_destinos "hola"','limitar_destinos verdadero','limitar_destinos 257,falso'):
            with self.assertRaises(ErrorLenguaje):compilar(self.fuente(orden))
    def test_sintaxis(self):
        for orden in ('limitar_destinos 257,','limitar_destinos ,257','limitar_destinos 1,2,3,4,5',
                      'limitar_destinos()','entero limitar_destinos = 1','numero limitar_destinos'):
            with self.assertRaises(ErrorLenguaje):compilar(self.fuente(orden))
    def test_expresiones(self):
        c=compilar(self.fuente('entero destino = 257\nlimitar_destinos destino, 257 + 1'))
        self.assertIn('nxk_es_limitar_destinos(UINT32_C(2)',c)
    def test_ejemplo(self):
        c=compilar((Path(__file__).parent/'ejemplos/destinos_mensajes.nxf').read_text())
        self.assertEqual(c.count('nxk_es_limitar_destinos('),2)

if __name__=='__main__':unittest.main()
