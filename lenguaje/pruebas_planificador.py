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
    def metricas():
        datos=comando('tareas estadisticas')
        patron=(rb'Tarea (\d+): turnos (\d+); cesiones (\d+); dormir (\d+)\r?\n'
                rb'  Envio (\d+); recepcion (\d+); espera msg/envio/fin (\d+)/(\d+)/(\d+); plazos (\d+); ciclos (\d+)')
        filas=[tuple(map(int,r)) for r in re.findall(patron,datos)]
        if len(filas)!=8:raise RuntimeError(datos.decode(errors='replace'))
        return filas
    def resumen(esperado,estados):
        datos=comando('tareas resumen')
        filas=re.search(rb'Planificador: registradas (\d+); pendientes (\d+); finalizadas (\d+)',datos)
        memoria=re.search(rb'Pilas reservadas (\d+); bytes (\d+); mensajes en buzones (\d+)',datos)
        grupos=re.findall(rb'(?:Listas/ejecutando/dormidas|Esperas mensaje/fin/envio|Terminadas/canceladas/libres) (\d+)/(\d+)/(\d+)',datos)
        if not filas or not memoria or tuple(map(int,filas.groups()+memoria.groups()))!=esperado or tuple(int(v) for g in grupos for v in g)!=estados:
            raise RuntimeError(datos.decode(errors='replace'))
        if b'Esperas semaforo 0' not in datos:raise RuntimeError('Espera de semaforo inesperada')
        if b'Esperas mutex 0' not in datos:raise RuntimeError('Espera de mutex inesperada')
        if comando('tareas resumen')!=datos:raise RuntimeError('Consultar resumen altera estado')
    def heap():
        datos=comando('mem')
        return int(re.search(rb'Heap fijo usado: (\d+)',datos)[1])
    try:
        esperar(0)
        version=re.search(r'#define NXK_VERSION "([^"]+)"',(raiz/'lib/version.h').read_text(encoding='utf-8'))[1].encode()
        comando('version',b'NXK '+version+b', nucleo propio x86 de 32 bits')
        resumen((0,0,0,0,0,0),(0,0,0,0,0,0,0,0,8))
        comando('tareas listar',b'Sin tareas registradas.')
        comando('tareas estadisticas',b'Sin tareas registradas.')
        for id in ('','0','-1','1x','4294967296','4294967295'):
            comando('tareas cancelar '+id,b'No se pudo cancelar')
        comando('tareas preparar',b'Ocho tareas preparadas')
        primeras=listar('lista',8); ids=[int(i) for i,_,_ in primeras]
        resumen((8,8,0,8,98304,0),(8,0,0,0,0,0,0,0,0))
        if len(set(ids))!=8 or any(int(a) for _,_,a in primeras):raise RuntimeError('IDs o estado inicial incorrectos')
        if any(any(r[1:]) for r in metricas()):raise RuntimeError('Contadores iniciales distintos de cero')
        usado=heap()
        comando('tareas preparar',b'No se pudo preparar')
        # El comando antiguo no debe sobrescribir un lote que ocupa los ocho slots.
        comando('tareas',b'Tareas no disponibles o fallo.')
        # Rechazar lotes sin slots tampoco debe filtrar objetos semaforo.
        for _ in range(10):
            comando('semaforos',b'No se pudo verificar sincronizacion.')
            comando('mutex',b'No se pudo verificar mutex.')
            comando('mutex recuperar',b'No se pudo recuperar el mutex.')
        resumen((8,8,0,8,98304,0),(8,0,0,0,0,0,0,0,0))
        comando('tareas cancelar '+str(ids[0]),b'Tarea cancelada.')
        paso=comando('tareas paso')
        if b'Presupuesto cumplido' not in paso and b'pendientes completadas' not in paso:raise RuntimeError('Paso invalido')
        comando('tareas ejecutar',b'Tareas pendientes completadas.')
        filas=listar(None,8)
        for id,estado,activaciones in filas:
            if int(id)==ids[0]:
                if estado!=b'cancelada' or activaciones!=b'0':raise RuntimeError('Tarea cancelada fue ejecutada')
            elif estado!=b'terminada' or activaciones!=b'5':raise RuntimeError('Finalizacion incorrecta')
        resumen((8,0,8,8,98304,0),(0,0,0,0,0,0,7,1,0))
        contadores=metricas()
        for r in contadores:
            esperado=(0,0,0,0,0,0,0,0,0,0) if r[0]==ids[0] else (5,0,4,0,0,0,0,0,0,0)
            if r[1:]!=esperado:raise RuntimeError('Contadores de tareas incorrectos: '+str(r))
        if metricas()!=contadores:raise RuntimeError('Consultar altera contadores')
        comando('tareas cancelar '+str(ids[1]),b'No se pudo cancelar')
        comando('tareas limpiar',b'Lote de tareas liberado.')
        comando('tareas listar',b'Sin tareas registradas.')
        comando('tareas preparar',b'Ocho tareas preparadas')
        resumen((8,8,0,8,98304,0),(8,0,0,0,0,0,0,0,0))
        nuevas=listar('lista',8)
        if any(any(r[1:]) for r in metricas()):raise RuntimeError('Nueva generacion conserva contadores')
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
        for _ in range(2):comando('tareas mensajes',b'Dieciseis mensajes: espera y despertar verificados.')
        for _ in range(2):comando('tareas resultados',b'Resultado 42 recibido por dos tareas; referencias verificadas.')
        for _ in range(2):comando('tareas flujo',b'Envio y recepcion con plazo verificados: 16 mensajes.')
        for _ in range(2):comando('tareas bloqueos',b'Ciclos de envio y finalizacion rechazados; buzones intactos.')
        resumen((0,0,0,8,98304,0),(0,0,0,0,0,0,0,0,8))
        for _ in range(3):comando('semaforos',b'Ocho tareas compartieron un recurso; 32 operaciones verificadas.')
        for _ in range(3):comando('mutex',b'Ocho tareas con propietario: 32 operaciones protegidas por mutex.')
        for _ in range(3):comando('mutex recuperar',b'Abandono detectado; datos reparados y mutex consistente.')
        memoria_antes=comando('mem')
        for _ in range(3):comando('espacios',b'Cuatro espacios privados verificados; paginas devueltas.')
        if comando('mem')!=memoria_antes:raise RuntimeError('Espacios alteran paginas, heap o guardas')
        if heap()!=usado:raise RuntimeError('La reutilizacion aumenta el heap')
        comando('usuario nextry',b'Aplicacion NEXTRY protegida termino.')
        comando('help',b'tareas cancelar ID')
        print('PLANIFICADOR OK: ocho tareas, pasos, estados, cancelacion, IDs, mensajes, resultados y memoria reutilizada.')
    finally:p.terminate();p.wait(timeout=5)
