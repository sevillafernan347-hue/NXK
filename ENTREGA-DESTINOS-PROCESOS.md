# Destinos permitidos de mensajes

Base2130103 (v0.0.26). Contrato aprobado por integracion para v0.0.27.

Cada proceso comienza sin filtro de destinos. Puede fijar hasta cuatro IDs
completos permitidos; las listas posteriores solo pueden ser subconjuntos,
sin duplicados. Lista vacia niega todos los envios. El permiso general de
ENVIAR_MENSAJES tambien es obligatorio. No cambia recepcion ni espera FIN.

IDs sintacticos: generacion1..0xffffff y slot1..4. No se exige que el destino
este vivo al configurar; un ID viejo no autoriza otra generacion del slot.
El filtro no retiene procesos ni consume slots. No se acepta origen kernel0.
Cancelar conserva politica; nueva generacion comienza sin filtro. Mensajes
ya encolados no se retiran. El kernel puede seguir enviar con origen0.

API nxk_proceso_limitar_destinos(id,const uint32_t *destinos,cantidad):0 exito,
-1 contrato/ID/duplicados/metadatos,-2 terminal/ejecutando,-3 contexto,
-5 ampliacion. Variante _actual solo durante INT81 del proceso EJECUTANDO.
nxk_proceso_destino_permitido(id,destino):1 permite,0 niega,-1 contrato;
consulta solo metadatos, sin resolver plazos ni exigir que destino exista.
Las listas son buffers kernel confiables y se copian antes de publicar.

Envios inmediatos y con espera comprueban filtro antes de consultar destino.
Revocar destino de un envio pendiente lo despierta con-5 al siguiente despacho,
sin encolar ni mantener una arista falsa en el detector de ciclos. Se vuelve a
comprobar incluso si esta LISTO por plazo/espacio; prevalece la revocacion.

INT81/27 EBX=tabla de DWORD ECX=cantidad0..4, validacion de todo el rango
legible antes de copiar. Cantidad0 no lee EBX. Retorno0,-1,-5; legacy-2.
NEXTRY limitar_destinos [expr_entera,...] con0..4 argumentos; un rechazo
termina explicitamente la app como limitar_archivos. ABI1 se conserva.
Consola proceso destinos ID [ID1,ID2,ID3,ID4]; sin lista retira todo envio.
proceso listar agrega destinos_restringidos y destinos_permitidos al final.

Puente C, una CPU y cuatro procesos. No son canales independientes ni una
politica de recepcion.

## Pruebas ejecutadas

100 pruebas compilador superadas; matriz QEMU x86 32/64/128/1024MiB y x64
64MiB superadas. Prueba interna verifica sintaxis, lista completa, reducciones,
rechazos atomicos, cancelacion, reciclaje y retirada de arista SEND al revocar.
Prueba externa verifica tabla RO cruzada, huecos/overflow/duplicados, servicio
legacy, envios permitidos/denegados, permiso general, lista vacia irreversible,
revocacion con buzon lleno y tras vencer, IDs reciclados, consola y NEXTRY.
Los recursos se recuperan y los discos snapshot no modifican originales.
Regresiones pruebas_permisos.py y pruebas_dependencias.py superadas, incluidos
ciclos, registros CPU, plazos, cancelacion y resultados retenidos.
Imagen x86 normal77836B/pruebas110604B; interfaz81932B; x64 normal24584B/
pruebas28680B. Los tamanos de archivo no representan consumo total de RAM.

MINI NXK se construye aparte y conserva el limite del cargador. La prueba
grafica valida captura VGA, teclado PS2, memoria, archivos, NEXTRY, procesos,
scroll y retroceso. La ventana abierta inicialmente usa la base estable0.0.26;
el lanzador de esta entrega construye el experimento sobre0.0.27.
