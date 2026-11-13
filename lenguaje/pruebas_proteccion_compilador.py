"""Contratos de aridad y tipos de la proteccion de datos privados."""
import sys
import unittest
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parent))
from compilador import compilar,ErrorLenguaje

class Proteccion(unittest.TestCase):
    def fuente(self,orden):return 'funcion inicio\n'+orden+'\nfin'
    def test_ejemplo(self):
        codigo=compilar((Path(__file__).parent/'ejemplos/proteccion_memoria.nxf').read_text())
        self.assertIn('nxk_es_proteger_paginas(',codigo)
        self.assertIn('nxk_es_escritura_pagina(',codigo)
    def test_tipos(self):
        for orden in ('numero proteger_paginas("x",2,0)', 'numero proteger_paginas(1,verdadero,0)',
                      'numero proteger_paginas(1,2,falso)', 'numero escritura_pagina("x")',
                      'logico x = escritura_pagina(1)'):
            with self.subTest(orden=orden),self.assertRaises(ErrorLenguaje):compilar(self.fuente(orden))
    def test_aridad(self):
        for orden in ('numero proteger_paginas(1,2)','numero proteger_paginas(1,2,0,0)',
                      'numero escritura_pagina()','numero escritura_pagina(1,2)'):
            with self.subTest(orden=orden),self.assertRaises(ErrorLenguaje):compilar(self.fuente(orden))
    def test_expresiones(self):
        self.assertIn('nxk_es_escritura_pagina(',compilar(self.fuente(
            'entero base = 0x402000\nafirmar proteger_paginas(base,1+1,0) == 0\nnumero escritura_pagina(base + 4096)')))
    def test_reservadas(self):
        for nombre in ('proteger_paginas','escritura_pagina'):
            with self.assertRaises(ErrorLenguaje):compilar(self.fuente('entero '+nombre+' = 0'))

if __name__=='__main__':unittest.main()
