# Buffers privados para servicios de procesos

Base: ae5f335 (v0.0.20). Contrato aprobado por la integracion para v0.0.21.

Los servicios INT81 existentes de salida, archivos, recepcion y resultados
aceptaran datos en paginas privadas reservadas, ademas de los rangos anteriores.
Lectura admite codigo/pila/datos; escritura solo pila/datos. Todo rango positivo
debe estar respaldado, incluso si atraviesa dos paginas. Huecos y overflow se
rechazan antes de copiar, modificar archivos o consumir mensajes. Las llamadas
de longitud cero conservan su contrato anterior.

La nueva API nxk_espacio_validar_rango permite validar sin copiar desde el mapa
kernel o el mismo espacio. Otro espacio privado no puede validar ese ID. Las
copias kernel siguen limitadas a CR3 kernel. No se modifican cabecera ni ABI1 NXE2.
Las rutas antiguas sin proceso gestionado conservan sus buffers anteriores.

Mientras una recepcion o espera de resultado conserva un buffer, ninguna de sus
paginas de datos puede liberarse por la API individual: devuelve -5, sin cambiar
el mapa ni el asignador. La retencion incluye LISTO tras vencer el plazo hasta
despachar el retorno. Cancelar, consumir o retirar al receptor elimina la espera.
Destruir todo el espacio receptor sigue devolviendo sus reservas; ya no puede
reanudar. Las referencias FIN hacia el proceso objetivo conservan su proteccion.

NEXTRY incorpora expresiones con retorno entero manejable:

- leer_archivo_en(nombre, direccion, cantidad, desplazamiento)
- guardar_archivo_desde(nombre, direccion, cantidad)
- mostrar_memoria(direccion, cantidad)
- recibir_en_memoria(direccion, ticks)
- esperar_en_memoria(destino, direccion, ticks)

Se usan los servicios existentes 5, 2, 1, 12 y 15. El kernel valida punteros,
longitudes, plazos y permisos; los retornos negativos se representan como uint32.
Las rutas sin soporte fallan explicitamente. Los ejemplos comentados se compilan
y ejecutan como procesos NXE2.

## Alcance y verificacion prevista

Puentes C para mapas, servicios y planificador; fuentes de ejemplo en NEXTRY.
Cuatro procesos, cuatro paginas de datos por espacio, una CPU, archivos RAM de
256 bytes, mensajes de dos enteros. No hay E/S de dispositivos asincrona,
transferencia de bloques IPC, ACL por archivo, NX ni certificacion ASUS.

Verificar cruces/huecos sin efectos parciales, permisos antes de punteros,
recepcion/FIN inmediata y suspendida, plazos, cancelacion, retencion de paginas,
reciclaje de IDs, memoria estable y regresiones de aplicaciones anteriores.
Registrar aqui solo resultados de pruebas efectivamente ejecutadas.

## Construccion y pruebas completadas

72 pruebas del compilador superadas:24 generales y48 de extensiones.
Matriz test.ps1 x86 superada con32,64,128 y1024MiB. Regresion x64 superada
con64MiB. Imagen x86 normal73740B y pruebas102412B; x64 normal24584B y
pruebas28680B. Los limites BIOS se conservan.

Para conservar las aserciones anteriores dentro de128KiB se aplica minsize
a las funciones de pruebas de espacios/procesos y a kernel_main de prueba.
La variante x64 tambien usa minsize en kernel_main para respetar su cargador
de32KiB; la compilacion x86 normal conserva -Os. No se amplian las reservas.

README se normalizo a UTF-8 para corregir dos comillas de la codificacion
anterior y permitir editar la documentacion de futuras versiones.

Prueba externa pruebas_buffers_procesos.py superada: servicios1..8 con nombres
y buffers en datos, cruces entre paginas y pila/datos, huecos sin efectos,
permisos antes de punteros, mensajes no consumidos al rechazar el buffer.
Recepcion inmediata, vacia, suspendida, por plazo y entrega tardia; FIN inmediato,
suspendido y por plazo, referencias protegidas y limpieza por cancelacion o
liberacion antigua. Los dos ejemplos NEXTRY reales terminan42. Heap y paginas
estables tras inicializar RAMFS; discos originales sin cambios con snapshot.
Pruebas internas verifican que ambas paginas de un buffer pendiente rechacen
liberacion con-5 para recepcion/FIN y vuelvan a liberarse tras cancelar.
Regresiones de memoria dinamica, permisos, dependencias y22 escenarios NXE2
desde disco superadas.

## Ejemplo de uso

```text
entero pagina = reservar_pagina()
afirmar pagina != 0
escribir_entero pagina, 0x616c6f68
afirmar guardar_archivo_desde("buf", pagina, 4) == 0
afirmar leer_archivo_en("buf", pagina, 4, 0) == 4
afirmar mostrar_memoria(pagina, 4) == 4
afirmar liberar_pagina(pagina) == 0
```

Las fuentes completas estan en lenguaje/ejemplos/buffer_archivos.nxf y
memoria_mensajes.nxf. Compilar como NXE2 con compilar-usuario.ps1 -Salida,
incluir en un volumen NXFS y cargar mediante proceso cargar. El segundo ejemplo
espera un mensaje de consola con valor42; usar proceso enviar ID 42 y continuar
con proceso paso ID. Consultar lenguaje/FORMATO-NXE-USUARIO.md.
