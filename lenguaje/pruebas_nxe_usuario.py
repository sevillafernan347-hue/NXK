"""Contrato del empaquetador NXE2, sin ejecutar el programa."""
import struct,sys,unittest
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parent))
from empaquetar_usuario import empaquetar,huella
class Formato(unittest.TestCase):
    def elf(self):
        e=bytearray(52);e[:7]=b'\x7fELF\x01\x01\x01';struct.pack_into('<HHII',e,16,2,3,1,0x400000);return e
    def test_cabecera_y_limite(self):
        datos=bytes(range(256))*15+b'x'*192
        nxe=empaquetar(self.elf(),datos)
        self.assertEqual(len(nxe),4096)
        self.assertEqual(struct.unpack_from('<4sHHIIIIIII',nxe),(b'NXE\0',2,1,64,0x400000,0,4032,8192,huella(datos),1))
        self.assertEqual(nxe[36:64],bytes(28));self.assertEqual(nxe[64:],datos)
    def test_rechazos(self):
        for datos in (b'',bytes(4033)):
            with self.assertRaises(ValueError):empaquetar(self.elf(),datos)
        for pos,v in ((0,0),(4,2),(5,2),(6,0),(16,1),(18,62),(20,0),(24,1)):
            e=self.elf();e[pos]=v
            with self.subTest(pos=pos),self.assertRaises(ValueError):empaquetar(e,b'codigo')
        with self.assertRaises(ValueError):empaquetar(b'ELF',b'codigo')
if __name__=='__main__':unittest.main(verbosity=2)
