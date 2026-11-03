"""Prueba interactivamente tareas generales y reutilizacion sin otra ventana QEMU."""
import pathlib,re,subprocess,time
raiz=pathlib.Path(__file__).resolve().parents[1]
log=raiz/'build/x86/planificador-consola.log'
with log.open('wb',buffering=0) as salida:
    p=subprocess.Popen([str(raiz/'tools/qemu/qemu-system-i386.exe'),'-machine','pc','-accel','tcg',
        '-cpu','qemu32','-m','32M','-drive','file=build/x86/nxk.img,format=raw,if=ide',
        '-snapshot','-display','none','-serial','stdio','-monitor','none','-no-reboot'],cwd=raiz,
        stdin=subprocess.PIPE,stdout=salida,stderr=salida)
    def esperar(desde):
        fin=time.monotonic()+15
        while time.monotonic()<fin:
            datos=log.read_bytes()[desde:]
            if b'nxk> ' in datos:return datos
            if p.poll() is not None:break
            time.sleep(.02)
        raise RuntimeError(log.read_text(errors='replace'))
    def comando(texto,esperado=None):
        desde=log.stat().st_size
        for v in (texto+'\n').encode():p.stdin.write(bytes([v]));p.stdin.flush();time.sleep(.003)
        datos=esperar(desde)
        if esperado is not None and esperado not in datos:raise RuntimeError(datos.decode(errors='replace'))
        return datos
    def listar(estado,cantidad):
        datos=comando('tareas listar')
        filas=re.findall(rb'Tarea (\d+): (\w+); activaciones (\d+)',datos)
        if len(filas)!=cantidad or (estado and any(e.decode()!=estado for _,e,_ in filas)):
            raise RuntimeError(datos.decode(errors='replace'))
        return filas
    def heap():
        datos=comando('mem')
        return int(re.search(rb'Heap fijo usado: (\d+)',datos)[1])
    try:
        esperar(0)
        comando('tareas listar',b'Sin tareas registradas.')
        for id in ('','0','-1','1x','4294967296','4294967295'):
            comando('tareas cancelar '+id,b'No se pudo cancelar')
        comando('tareas preparar',b'Ocho tareas preparadas')
        primeras=listar('lista',8); ids=[int(i) for i,_,_ in primeras]
        if len(set(ids))!=8 or any(int(a) for _,_,a in primeras):raise RuntimeError('IDs o estado inicial incorrectos')
        usado=heap()
        comando('tareas preparar',b'No se pudo preparar')
        # El comando antiguo no debe sobrescribir un lote que ocupa los ocho slots.
        comando('tareas',b'Tareas no disponibles o fallo.')
        comando('tareas cancelar '+str(ids[0]),b'Tarea cancelada.')
        paso=comando('tareas paso')
        if b'Presupuesto cumplido' not in paso and b'pendientes completadas' not in paso:raise RuntimeError('Paso invalido')
        comando('tareas ejecutar',b'Tareas pendientes completadas.')
        filas=listar(None,8)
        for id,estado,activaciones in filas:
            if int(id)==ids[0]:
                if estado!=b'cancelada' or activaciones!=b'0':raise RuntimeError('Tarea cancelada fue ejecutada')
            elif estado!=b'terminada' or activaciones!=b'5':raise RuntimeError('Finalizacion incorrecta')
        comando('tareas cancelar '+str(ids[1]),b'No se pudo cancelar')
        comando('tareas limpiar',b'Lote de tareas liberado.')
        comando('tareas listar',b'Sin tareas registradas.')
        comando('tareas preparar',b'Ocho tareas preparadas')
        nuevas=listar('lista',8)
        if set(ids)&{int(i) for i,_,_ in nuevas}:raise RuntimeError('Reaparecio un ID antiguo')
        comando('tareas cancelar '+str(ids[0]),b'No se pudo cancelar')
        comando('tareas ejecutar',b'Tareas pendientes completadas.')
        listar('terminada',8)
        comando('tareas limpiar',b'Lote de tareas liberado.')
        # Limpiar tareas que aun no ejecutaron tambien libera sus argumentos.
        comando('tareas preparar',b'Ocho tareas preparadas')
        comando('tareas limpiar',b'Lote de tareas liberado.')
        comando('tareas listar',b'Sin tareas registradas.')
        for _ in range(2):comando('tareas ocho',b'Ocho tareas: argumentos, pilas, esperas y finalizacion verificadas.')
        if heap()!=usado:raise RuntimeError('La reutilizacion aumenta el heap')
        comando('usuario nextry',b'Aplicacion NEXTRY protegida termino.')
        comando('help',b'tareas cancelar ID')
        print('PLANIFICADOR OK: ocho tareas, pasos, estados, cancelacion, IDs y memoria reutilizada.')
    finally:p.terminate();p.wait(timeout=5)
