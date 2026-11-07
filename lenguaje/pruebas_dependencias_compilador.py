"""Tipos y contratos de espera de envio, resultado de proceso y finalizar."""
import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from compilador import compilar, ErrorLenguaje


class DependenciasNextry(unittest.TestCase):
    declaraciones = 'entero destino = 1\nentero valor = 2\nentero estado = 3\nentero codigo = 4\nentero resultado = 5'

    def fuente(self, orden, declaraciones=None):
        return 'funcion inicio\n' + (self.declaraciones if declaraciones is None else declaraciones) + '\n' + orden + '\nfin'

    def error(self, orden, mensaje, declaraciones=None):
        with self.assertRaisesRegex(ErrorLenguaje, mensaje):
            compilar(self.fuente(orden, declaraciones))

    def test_ejemplos(self):
        for nombre in ('proceso_productor.nxf', 'proceso_consumidor.nxf', 'proceso_observador.nxf'):
            with self.subTest(nombre=nombre):
                fuente = (Path(__file__).parent/'ejemplos'/nombre).read_text(encoding='utf-8')
                self.assertIn('nxk_es_finalizar(', compilar(fuente))

    def test_expresa_envio_y_espera(self):
        codigo = compilar(self.fuente('enviar_esperando destino + 1, valor + 2, resultado, 0\nesperar_proceso destino, estado, codigo, resultado, 100\nfinalizar codigo'))
        self.assertIn('nxk_es_enviar_esperando(', codigo)
        self.assertRegex(codigo, r'= nxk_es_esperar_proceso\([^,]+, &\w+, &\w+, UINT32_C\(100\)\);')
        self.assertIn('nxk_es_finalizar(', codigo)

    def test_envio_exige_destino_valor_y_plazo_enteros(self):
        for orden in ('enviar_esperando falso, valor, resultado, 1',
                      'enviar_esperando destino, "texto", resultado, 1',
                      'enviar_esperando destino, valor, resultado, verdadero'):
            with self.subTest(orden=orden):
                self.error(orden, 'entero')

    def test_espera_exige_destino_y_plazo_enteros(self):
        for orden in ('esperar_proceso falso, estado, codigo, resultado, 1',
                      'esperar_proceso destino, estado, codigo, resultado, "texto"'):
            self.error(orden, 'entero')

    def test_salidas_exigen_entero(self):
        for nombre in ('estado', 'codigo', 'resultado'):
            for tipo, valor in (('logico', 'falso'), ('texto', '"hola"')):
                declaraciones = self.declaraciones.replace('entero '+nombre+' = '+{'estado':'3','codigo':'4','resultado':'5'}[nombre], tipo+' '+nombre+' = '+valor)
                self.error('esperar_proceso destino, estado, codigo, resultado, 1', 'entero', declaraciones)
                if nombre == 'resultado':
                    self.error('enviar_esperando destino, valor, resultado, 1', 'entero', declaraciones)

    def test_salidas_rechazan_constantes(self):
        for nombre in ('estado', 'codigo', 'resultado'):
            declaraciones = self.declaraciones.replace('entero '+nombre, 'constante entero '+nombre)
            self.error('esperar_proceso destino, estado, codigo, resultado, 1', 'variable', declaraciones)
            if nombre == 'resultado':
                self.error('enviar_esperando destino, valor, resultado, 1', 'variable', declaraciones)

    def test_espera_rechaza_alias(self):
        for destinos in ('estado, estado, resultado', 'estado, codigo, estado', 'estado, codigo, codigo'):
            self.error('esperar_proceso destino, '+destinos+', 1', 'distintas')

    def test_salida_debe_existir_y_estar_en_ambito(self):
        self.error('enviar_esperando destino, valor, ausente, 1', 'no existe')
        self.error('esperar_proceso destino, estado, ausente, resultado, 1', 'no existe')
        self.error('si verdadero\nentero local = 0\nfin\nesperar_proceso destino, estado, local, resultado, 1', 'no existe')

    def test_finalizar_exige_codigo_entero(self):
        for expresion in ('falso', '"42"'):
            self.error('finalizar '+expresion, 'entero')
        self.assertIn('nxk_es_finalizar(UINT32_C(4294967295));', compilar(self.fuente('finalizar 0xffffffff')))

    def test_cantidad_y_forma_de_argumentos(self):
        for orden in ('enviar_esperando destino, valor, resultado',
                      'esperar_proceso destino, estado, codigo, resultado'):
            self.error(orden, ',')
        for orden in ('enviar_esperando destino, valor, resultado, 1, 2',
                      'esperar_proceso destino, estado, codigo, resultado, 1, 2',
                      'finalizar 1, 2'):
            self.error(orden, 'NL')
        for orden in ('enviar_esperando destino, valor, 1, 2',
                      'esperar_proceso destino, estado + 1, codigo, resultado, 1'):
            self.error(orden, 'NOMBRE|,')
        self.error('finalizar', 'expresion')

    def test_palabras_reservadas(self):
        for nombre in ('enviar_esperando', 'esperar_proceso', 'finalizar'):
            self.error('', 'NOMBRE', 'entero '+nombre+' = 0')

    def test_esperas_en_bloques_y_finalizar_en_funcion(self):
        fuente = self.fuente('mientras verdadero\nenviar_esperando destino, valor, resultado, 0\nesperar_proceso destino, estado, codigo, resultado, 0\nromper\nfin\nllamar terminar')
        fuente += '\nfuncion terminar\nfinalizar 42\nfin'
        self.assertIn('nxk_es_finalizar(UINT32_C(42));', compilar(fuente))


if __name__ == '__main__':
    unittest.main(verbosity=2)
