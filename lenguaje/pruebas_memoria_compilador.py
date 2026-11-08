"""Sintaxis y tipos de paginas privadas de proceso desde NEXTRY."""
import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from compilador import compilar, ErrorLenguaje


class MemoriaNextry(unittest.TestCase):
    def fuente(self, orden):
        return 'funcion inicio\n' + orden + '\nfin'

    def error(self, orden, patron):
        with self.assertRaisesRegex(ErrorLenguaje, patron):
            compilar(self.fuente(orden))

    def test_ejemplo_real(self):
        fuente = (Path(__file__).parent/'ejemplos/pagina_memoria.nxf').read_text(encoding='utf-8')
        codigo = compilar(fuente)
        for nombre in ('reservar_pagina', 'liberar_pagina', 'leer_entero', 'escribir_entero'):
            self.assertIn('nxk_es_' + nombre + '(', codigo)
        self.assertIn('nxk_es_finalizar(UINT32_C(42))', codigo)

    def test_reserva_entera_sin_argumentos(self):
        codigo = compilar(self.fuente('entero pagina = reservar_pagina()\nafirmar pagina != 0'))
        self.assertIn('nxk_es_reservar_pagina()', codigo)
        self.error('numero reservar_pagina', r'\(')
        self.error('numero reservar_pagina(1)', r'\)')
        self.error('logico pagina = reservar_pagina()', 'logico')

    def test_consultas_usan_un_argumento_entero(self):
        for nombre in ('liberar_pagina', 'leer_entero'):
            with self.subTest(nombre=nombre):
                self.assertIn('nxk_es_' + nombre + '(UINT32_C(4202496))',
                              compilar(self.fuente('entero valor = '+nombre+'(0x402000)')))
                self.error('numero '+nombre+'()', 'expresion')
                self.error('numero '+nombre+'(1, 2)', r'\)')
                self.error('numero '+nombre+'(verdadero)', 'entero')
                self.error('numero '+nombre+'("direccion")', 'entero')

    def test_liberacion_devuelve_estado_sin_signo(self):
        codigo = compilar(self.fuente('afirmar liberar_pagina(0) == 0xffffffff'))
        self.assertIn('nxk_es_liberar_pagina(UINT32_C(0))', codigo)
        self.assertIn('UINT32_C(4294967295)', codigo)

    def test_escritura_exige_dos_enteros(self):
        self.assertIn('nxk_es_escribir_entero(UINT32_C(4202496), UINT32_C(42))',
                      compilar(self.fuente('escribir_entero 0x402000, 42')))
        for orden in ('escribir_entero verdadero, 1', 'escribir_entero 1, falso',
                      'escribir_entero "direccion", 1', 'escribir_entero 1, "valor"'):
            self.error(orden, 'entero')

    def test_escritura_completa_en_una_linea(self):
        self.error('escribir_entero', 'expresion')
        self.error('escribir_entero 1', ',')
        self.error('escribir_entero 1,', 'expresion')
        self.error('escribir_entero 1, 2, 3', 'NL')
        self.error('numero escribir_entero', 'expresion')
        self.error('liberar_pagina(0)', 'Orden no reconocida')

    def test_palabras_reservadas(self):
        for nombre in ('reservar_pagina', 'liberar_pagina', 'leer_entero', 'escribir_entero'):
            self.error('entero '+nombre+' = 0', 'NOMBRE')

    def test_expresiones_dentro_de_bloques(self):
        codigo = compilar(self.fuente(
            'entero pagina = reservar_pagina()\n'
            'si pagina != 0\n'
            'repetir 2 veces\n'
            'escribir_entero pagina + 4, leer_entero(pagina) + 1\n'
            'fin\n'
            'afirmar liberar_pagina(pagina) == 0\n'
            'fin'))
        self.assertIn('nxk_es_escribir_entero(', codigo)
        self.assertIn('nxk_es_leer_entero(', codigo)


if __name__ == '__main__':
    unittest.main(verbosity=2)
