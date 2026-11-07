"""Contratos de tipos y salidas del IPC de procesos en NEXTRY."""
import sys
import unittest
from pathlib import Path
sys.path.insert(0, str(Path(__file__).resolve().parent))
from compilador import compilar, ErrorLenguaje


class MensajesNextry(unittest.TestCase):
    def programa(self, orden, declaraciones='entero v = 0\nentero ori = 0\nentero r = 0'):
        return 'funcion inicio\n' + declaraciones + '\n' + orden + '\nfin'

    def error(self, orden, mensaje, declaraciones='entero v = 0\nentero ori = 0\nentero r = 0'):
        with self.assertRaisesRegex(ErrorLenguaje, mensaje):
            compilar(self.programa(orden, declaraciones))

    def test_ejemplos(self):
        for nombre in ('proceso_mensajes.nxf', 'proceso_receptor.nxf'):
            with self.subTest(nombre=nombre):
                fuente=(Path(__file__).parent/'ejemplos'/nombre).read_text(encoding='utf-8')
                codigo=compilar(fuente)
                self.assertIn('nxk_es_recibir_mensaje(', codigo)
                self.assertIn('nxk_es_proceso_actual()', codigo)

    def test_orden_de_argumentos(self):
        codigo=compilar(self.programa('recibir_mensaje v, ori, r, 30'))
        self.assertIn('n_72_3 = nxk_es_recibir_mensaje(&n_6f7269_2, &n_76_1, UINT32_C(30));', codigo)

    def test_expresiones_enteras(self):
        codigo=compilar(self.programa('enviar_mensaje proceso_actual(), v + 1\nrecibir_mensaje v, ori, r, r + 20'))
        self.assertIn('nxk_es_enviar_mensaje(nxk_es_proceso_actual(),', codigo)
        self.assertIn('nxk_es_recibir_mensaje(', codigo)
        self.assertIn('nxk_es_memoria_usada()',compilar(self.programa('numero memoria_usada()')))

    def test_envio_exige_enteros(self):
        for orden in ('enviar_mensaje "uno", 1', 'enviar_mensaje 1, verdadero'):
            self.error(orden, 'entero')

    def test_recepcion_exige_tres_enteros(self):
        for variable in ('v', 'ori', 'r'):
            for tipo, valor in (('logico', 'falso'), ('texto', '"hola"')):
                declaraciones='\n'.join(f'{tipo if n==variable else "entero"} {n} = {valor if n==variable else "0"}' for n in ('v','ori','r'))
                with self.subTest(variable=variable, tipo=tipo):
                    self.error('recibir_mensaje v, ori, r, 0', 'entero', declaraciones)

    def test_recepcion_rechaza_constantes(self):
        for variable in ('v','ori','r'):
            declaraciones='\n'.join(f'{"constante " if n==variable else ""}entero {n} = 0' for n in ('v','ori','r'))
            self.error('recibir_mensaje v, ori, r, 0', 'variable', declaraciones)

    def test_recepcion_rechaza_alias(self):
        for destinos in ('v, v, r', 'v, ori, v', 'v, ori, ori'):
            self.error('recibir_mensaje '+destinos+', 0', 'distintas')

    def test_recepcion_plazo_entero(self):
        for plazo in ('falso', '"diez"'):
            self.error('recibir_mensaje v, ori, r, '+plazo, 'entero')

    def test_recepcion_ambito(self):
        self.error('recibir_mensaje inexistente, ori, r, 0', 'no existe')
        self.error('si verdadero\nentero local = 0\nfin\nrecibir_mensaje v, local, r, 0', 'no existe')

    def test_estructura_y_argumentos(self):
        for orden in ('enviar_mensaje 1', 'recibir_mensaje v, ori, r'):
            self.error(orden, ',')
        for orden in ('recibir_mensaje v + 1, ori, r, 0', 'recibir_mensaje 1, ori, r, 0'):
            self.error(orden, 'NOMBRE|,')
        for orden in ('enviar_mensaje 1, 2, 3', 'recibir_mensaje v, ori, r, 0, 1'):
            self.error(orden, 'NL')
        self.error('numero proceso_actual', r'\(')
        self.error('numero proceso_actual(1)', r'\)')

    def test_palabras_reservadas(self):
        for palabra in ('enviar_mensaje','recibir_mensaje','proceso_actual'):
            self.error('', 'NOMBRE', f'entero {palabra} = 0')

    def test_recibir_en_bloque(self):
        codigo=compilar(self.programa('mientras verdadero\nrecibir_mensaje v, ori, r, 0\nromper\nfin'))
        self.assertIn('nxk_es_recibir_mensaje(', codigo)


if __name__ == '__main__':
    unittest.main(verbosity=2)
