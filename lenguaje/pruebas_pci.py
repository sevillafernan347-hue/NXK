"""Contrasta inventario PCI con QMP, incluyendo puente y funciones discontinuas."""
import pathlib,subprocess,socket,time,json,re
raiz=pathlib.Path(__file__).resolve().parents[1]
carpeta=raiz/'build/x86/pci-pruebas';carpeta.mkdir(exist_ok=True)
original=(raiz/'build/x86/nxk.img').read_bytes()
casos={'basico':[], 'puente-multifuncion':['-device','pci-bridge,id=puente,chassis_nr=1,addr=5',
    '-device','e1000,addr=4.0,multifunction=on','-device','e1000,addr=4.3',
    '-device','rtl8139,bus=puente,addr=2']}
for nombre,extra in casos.items():
    with socket.socket() as reserva:reserva.bind(('127.0.0.1',0));puerto=reserva.getsockname()[1]
    log=carpeta/(nombre+'.log')
    with log.open('wb',buffering=0) as salida:
        p=subprocess.Popen([str(raiz/'tools/qemu/qemu-system-i386.exe'),'-machine','pc','-accel','tcg',
            '-cpu','qemu32','-m','32M','-nic','none','-drive','file=build/x86/nxk.img,format=raw,if=ide,snapshot=on',
            '-display','none','-serial','stdio','-monitor','none','-no-reboot',
            '-qmp',f'tcp:127.0.0.1:{puerto},server=on,wait=off']+extra,cwd=raiz,
            stdin=subprocess.PIPE,stdout=salida,stderr=salida)
        canal=None
        def esperar(desde):
            limite=time.monotonic()+20
            while time.monotonic()<limite:
                datos=log.read_bytes()[desde:]
                if b'nxk> ' in datos:return datos
                if p.poll() is not None:break
                time.sleep(.02)
            raise AssertionError(log.read_text(errors='replace'))
        def comando(c):
            desde=log.stat().st_size
            for v in (c+'\n').encode():p.stdin.write(bytes([v]));p.stdin.flush();time.sleep(.003)
            return esperar(desde)
        try:
            esperar(0);canal=socket.create_connection(('127.0.0.1',puerto),timeout=5);flujo=canal.makefile('rwb',buffering=0)
            assert 'QMP' in json.loads(flujo.readline())
            def qmp(c):
                flujo.write((json.dumps({'execute':c})+'\n').encode())
                while True:
                    r=json.loads(flujo.readline())
                    if 'return' in r:return r['return']
                    assert 'error' not in r,r
            qmp('qmp_capabilities');antes=qmp('query-pci')
            esperado={}
            def recorrer(dispositivos):
                for d in dispositivos:
                    esperado[(d['bus'],d['slot'],d['function'])]=(d['id']['device']<<16|d['id']['vendor'],d['class_info']['class'])
                    if 'pci_bridge' in d:recorrer(d['pci_bridge'].get('devices',[]))
            for bus in antes:recorrer(bus['devices'])
            memoria=comando('mem')
            for _ in range(3):
                inventario=comando('pci');observado={}
                for b,d,f,id,clase in re.findall(rb'PCI (\d+):(\d+)\.(\d+) ID 0x([0-9A-F]+) clase 0x([0-9A-F]+)',inventario):
                    clave=(int(b),int(d),int(f));assert clave not in observado
                    observado[clave]=(int(id,16),int(clase,16)>>8)
                assert observado==esperado,(nombre,observado,esperado,inventario)
                assert b'; omitidos 0' in inventario
            assert comando('mem')==memoria
            assert qmp('query-pci')==antes,'El inventario altero dispositivos/recursos'
            if extra:assert any(b>0 for b,d,f in observado) and (0,4,3) in observado
            assert b'Aplicacion NEXTRY protegida termino.' in comando('usuario nextry')
            assert b'help | version' in comando('help')
            print('PCI '+nombre+': coincide con QMP, recursos y heap estables.',flush=True)
        finally:
            if canal:canal.close()
            p.terminate();p.wait(timeout=5)
assert (raiz/'build/x86/nxk.img').read_bytes()==original
