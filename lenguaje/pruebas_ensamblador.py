# GUIA DE LECTURA NXK - lenguaje/pruebas_ensamblador.py
# Comprueba los bytes de instrucciones, desplazamientos de saltos y rechazo de operandos incorrectos.
# Contexto y orden de estudio: ver LEER-CODIGO.md en la carpeta principal.
"""Pruebas de codificacion y rechazo de entradas para NEXTRY ASM."""
import unittest
import sys
from pathlib import Path
# Python portatil no agrega automaticamente esta carpeta a sus importaciones.
sys.path.insert(0, str(Path(__file__).resolve().parent))
from ensamblador import ensamblar


class Ensamblador(unittest.TestCase):
    def test_instrucciones_mapa_bios(self):
        fuente='''modo 16
comparar_inmediato32 eax, 0x534d4150
sumar_inmediato16 bp, 1
comparar_inmediato16 bp, 32
escribir_memoria16 0x5000, bp
guardar16 di
recuperar16 di
'''
        self.assertEqual(ensamblar(fuente), bytes.fromhex(
            '66 81 f8 50 41 4d 53 81 c5 01 00 81 fd 20 00 89 2e 00 50 57 5f'))

    def test_operaciones32(self):
        fuente = '''modo 32
comparar32 edi, esi
sumar32 edi, ecx
decrementar32 esi
direccion_atras
copiar_bytes
direccion_adelante
devolver
'''
        self.assertEqual(ensamblar(fuente), bytes.fromhex('39 f7 01 cf 4e fd f3 a4 fc c3'))

    def test_saltos_adelante_atras(self):
        fuente = '''modo 32
inicio:
si_igual fin
si_menor_igual_sin_signo inicio
fin:
devolver
'''
        self.assertEqual(ensamblar(fuente), bytes.fromhex('0f 84 06 00 00 00 0f 86 f4 ff ff ff c3'))

    def test_rechazar_modo_y_operandos(self):
        for fuente in ('modo 16\nsumar32 eax, ecx', 'modo 32\ncomparar32 ax, cx',
                       'modo 32\ndireccion_atras eax', 'modo 32\nsumar32 eax',
                       'modo 32\ndecrementar32 eax, ebx', 'modo 32\nsi_igual falta'):
            with self.subTest(fuente=fuente), self.assertRaises(ValueError):
                ensamblar(fuente)


if __name__ == '__main__':
    unittest.main(verbosity=2)
