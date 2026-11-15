"""Compila el controlador con puertos simulados; sin acceso a discos o puertos reales."""
from pathlib import Path
import subprocess

raiz = Path(__file__).resolve().parents[1]
salida = raiz / 'build/ata-estados'
salida.mkdir(parents=True, exist_ok=True)
exe = salida / 'ata_estados.exe'
subprocess.run([str(raiz / 'tools/zig/zig.exe'), 'cc', '-O2', '-Wall', '-Wextra',
                '-Werror', '-I', str(raiz / 'lib'), str(raiz / 'pruebas/ata_estados.c'),
                '-o', str(exe)], cwd=raiz, check=True)
subprocess.run([str(exe)], cwd=raiz, check=True, timeout=30)
