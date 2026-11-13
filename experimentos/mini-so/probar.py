"""Prueba la pantalla VGA y comandos reales en QEMU sin cerrar ventanas del usuario."""
import json
import socket
import struct
import subprocess
import time
import zlib
from pathlib import Path
RAIZ=Path(__file__).resolve().parents[2]

def main():
    carpeta=RAIZ/'build/x86-interfaz'; disco=carpeta/'nxk.img'
    original=disco.read_bytes(); normal=RAIZ/'build/x86/nxk.img'
    normal_antes=normal.read_bytes()
    with socket.socket() as libre:
        libre.bind(('127.0.0.1',0)); puerto=libre.getsockname()[1]
    log=carpeta/'prueba-grafica.log'
    salida=log.open('wb',buffering=0)
    p=subprocess.Popen([str(RAIZ/'tools/qemu/qemu-system-i386.exe'),'-machine','pc',
        '-cpu','qemu32','-accel','tcg','-m','64M','-drive',
        f'file={disco.as_posix()},format=raw,if=ide,index=0,snapshot=on',
        '-display','none','-serial','stdio','-monitor','none','-no-reboot',
        '-qmp',f'tcp:127.0.0.1:{puerto},server=on,wait=off'],cwd=RAIZ,
        stdin=subprocess.PIPE,stdout=salida,stderr=salida)
    conexion=None
    def esperar(desde,marca=b'nxk> '):
        limite=time.monotonic()+20
        while time.monotonic()<limite:
            datos=log.read_bytes()[desde:]
            if marca in datos:return datos
            if p.poll() is not None:break
            time.sleep(.02)
        raise AssertionError(log.read_text(errors='replace'))
    def comando(orden,marca):
        inicio=log.stat().st_size
        for b in (orden+'\n').encode():
            p.stdin.write(bytes([b]));p.stdin.flush();time.sleep(.005)
        assert marca in esperar(inicio),(orden,log.read_text(errors='replace'))
    try:
        esperar(0)
        conexion=socket.create_connection(('127.0.0.1',puerto),timeout=5)
        qmp=conexion.makefile('rwb',buffering=0); assert 'QMP' in json.loads(qmp.readline())
        def pedir(orden,args=None):
            peticion={'execute':orden}
            if args is not None:peticion['arguments']=args
            qmp.write((json.dumps(peticion)+'\n').encode())
            while True:
                respuesta=json.loads(qmp.readline())
                if 'return' in respuesta:return respuesta['return']
                assert 'error' not in respuesta,respuesta
        pedir('qmp_capabilities')
        ppm=carpeta/'mini-nxk.ppm'
        pedir('screendump',{'filename':str(ppm)})
        cabecera,pixeles=ppm.read_bytes().split(b'\n255\n',1)
        campos=cabecera.split(); ancho,alto=map(int,campos[1:])
        # QEMU duplica pixels/lineas VGA al presentar modo13h (640x400).
        assert campos[0]==b'P6' and (ancho,alto) in ((320,200),(640,400))
        assert len(pixeles)==ancho*alto*3
        colores={pixeles[i:i+3] for i in range(0,len(pixeles),3)}
        assert len(colores)>=6,colores
        # Conversion de la captura PPM a PNG con biblioteca estandar.
        def bloque(tipo,datos):
            return struct.pack('>I',len(datos))+tipo+datos+struct.pack('>I',zlib.crc32(tipo+datos)&0xffffffff)
        filas=b''.join(b'\0'+pixeles[y*ancho*3:(y+1)*ancho*3] for y in range(alto))
        png=b'\x89PNG\r\n\x1a\n'+bloque(b'IHDR',struct.pack('>IIBBBBB',ancho,alto,8,2,0,0,0))
        png+=bloque(b'IDAT',zlib.compress(filas))+bloque(b'IEND',b'')
        (carpeta/'mini-nxk.png').write_bytes(png)
        # Inyectar teclas PS/2 mediante el emulador, no entrada serie.
        desde=log.stat().st_size
        for tecla in ('v','e','r','s','i','o','n','ret'):
            pedir('human-monitor-command',{'command-line':'sendkey '+tecla})
            time.sleep(.09)
        assert b'NXK 0.0.27' in esperar(desde)
        comando('mem',b'Heap fijo usado')
        comando('escribir grafico hola',b'Archivo guardado en RAM.')
        comando('leer grafico',b'hola')
        comando('usuario nextry',b'NEXTRY')
        comando('proceso demo',b'registros y punto de ejecucion conservados')
        # Fuerza varias vueltas de scroll y un retroceso al escribir.
        comando('help',b'proceso permisos'); comando('versionx\b',b'NXK 0.0.27')
    finally:
        if conexion:conexion.close()
        p.terminate();p.wait(timeout=5);salida.close()
        assert disco.read_bytes()==original and normal.read_bytes()==normal_antes
    print('INTERFAZ OK: VGA320x200, captura, teclado PS2, archivos, NEXTRY, procesos y scroll; discos intactos.')

if __name__=='__main__':main()
