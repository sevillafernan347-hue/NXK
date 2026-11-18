"""Consulta diagnosticos ATA en QEMU sin escribir discos ni usar la ventana del usuario."""
from pathlib import Path
import re
import subprocess
import time

raiz = Path(__file__).resolve().parents[1]
imagen = raiz / 'build/x86/nxk.img'
original = imagen.read_bytes()
log = raiz / 'build/x86/ata-diagnostico-consola.log'
with log.open('wb', buffering=0) as salida:
    proceso = subprocess.Popen([
        str(raiz / 'tools/qemu/qemu-system-i386.exe'), '-machine', 'pc',
        '-accel', 'tcg', '-cpu', 'qemu32', '-m', '32M',
        '-drive', f'file={imagen.as_posix()},format=raw,if=ide,snapshot=on',
        '-display', 'none', '-serial', 'stdio', '-monitor', 'none', '-no-reboot',
    ], cwd=raiz, stdin=subprocess.PIPE, stdout=salida, stderr=salida)

    def esperar(desde):
        plazo = time.monotonic() + 15
        while time.monotonic() < plazo:
            datos = log.read_bytes()[desde:]
            if b'nxk> ' in datos:
                return datos
            if proceso.poll() is not None:
                break
            time.sleep(.02)
        raise AssertionError(log.read_text(errors='replace'))

    def comando(texto):
        desde = log.stat().st_size
        for byte in (texto + '\n').encode():
            proceso.stdin.write(bytes([byte]))
            proceso.stdin.flush()
            time.sleep(.003)
        return esperar(desde)

    try:
        esperar(0)
        inicial = comando('discos estado')
        assert inicial.count(b'sin operaciones.') == 2, inicial
        listado = comando('discos')
        assert b'sectores de 512 bytes: 2048' in listado, listado
        assert b'IDE 1: no disponible o no compatible.' in listado, listado
        estado = comando('discos estado')
        assert re.search(rb'IDE 0 comando 0x000000EC emitido 1 LBA 0 fase cierre resultado 0 estado', estado), estado
        assert re.search(rb'IDE 1 comando 0x000000EC emitido 0 LBA 0 fase previa resultado -2 estado', estado), estado
        # La consulta no debe reidentificar ni alterar siquiera el contador de sondeos.
        for _ in range(3):
            assert comando('discos estado') == estado
        assert b'discos estado' in comando('help')
        print('ATA CONSOLA OK: inicio vacio, maestro/esclavo, resultado firmado y consultas estables.')
    finally:
        proceso.terminate()
        proceso.wait(timeout=5)
assert imagen.read_bytes() == original, 'La imagen de arranque cambio'
print('ATA CONSOLA OK: disco original intacto.')
