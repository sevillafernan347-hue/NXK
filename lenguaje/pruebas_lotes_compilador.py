"""Tipos y aridad de reservas/liberaciones por lotes."""
import sys
import unittest
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parent))
from compilador import compilar,ErrorLenguaje

class Lotes(unittest.TestCase):
    def fuente(self,orden):return 'funcion inicio\n'+orden+'\nfin'
    def test_ejemplo(self):
        self.assertIn('nxk_es_liberar_paginas(',compilar((Path(__file__).parent/'ejemplos/paginas_lote.nxf').read_text()))
    def test_tipos(self):
        for orden in ('numero reservar_paginas(verdadero)','numero liberar_paginas("x",2)',
                      'numero liberar_paginas(1,falso)','logico x = reservar_paginas(2)'):
            with self.subTest(orden=orden),self.assertRaises(ErrorLenguaje):compilar(self.fuente(orden))
    def test_aridad(self):
        for orden in ('numero reservar_paginas()','numero reservar_paginas(1,2)',
                      'numero liberar_paginas(1)','numero liberar_paginas(1,2,3)'):
            with self.subTest(orden=orden),self.assertRaises(ErrorLenguaje):compilar(self.fuente(orden))
    def test_expresiones(self):
        codigo=compilar(self.fuente('entero base = reservar_paginas(1 + 1)\nafirmar liberar_paginas(base,2) == 0'))
        self.assertIn('nxk_es_reservar_paginas(',codigo)
    def test_reservadas(self):
        for nombre in ('reservar_paginas','liberar_paginas'):
            with self.assertRaises(ErrorLenguaje):compilar(self.fuente('entero '+nombre+' = 0'))

if __name__=='__main__':unittest.main()
