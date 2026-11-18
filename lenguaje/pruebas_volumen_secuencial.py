"""Compila y prueba el lector NXFS real con un disco simulado."""
from pathlib import Path
import subprocess
import sys

sys.path.insert(0, str(Path(__file__).resolve().parent))
from crear_volumen import crear

raiz = Path(__file__).resolve().parents[1]
salida = raiz / 'build/nxfs-host'
salida.mkdir(parents=True, exist_ok=True)
fixture = salida / 'prueba.img'
fixture.write_bytes(crear({'a': bytes(i % 251 for i in range(4096)), 'b': b'texto'}))
exe = salida / 'volumen_secuencial.exe'
subprocess.run([
    str(raiz / 'tools/zig/zig.exe'), 'cc', '-O2', '-UNDEBUG', '-Wall', '-Wextra', '-Werror',
    '-I', str(raiz / 'lib'), str(raiz / 'pruebas/volumen_secuencial.c'),
    str(raiz / 'kernel/volumen.c'), '-o', str(exe),
], cwd=raiz, check=True)
subprocess.run([str(exe), str(fixture)], cwd=raiz, check=True, timeout=15)
