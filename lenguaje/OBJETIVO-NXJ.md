# Objetivo de NXJ: capacidades de Java antes de divergencias propias

Requisito del usuario: ofrecer la comodidad y las capacidades de Java, incluidos hilos y el resto de sus componentes, antes de desarrollar una variante mas independiente de NXK.

No basta con traducir palabras clave ni con aceptar sintaxis que despues no tenga comportamiento real. Cada capacidad debe tener implementacion, pruebas y limites documentados.

## Estado actual

NXJ 0.1 implementa una clase organizativa por archivo, metodos publicos estaticos sin argumentos ni retorno y las operaciones basicas de NXK Espanol. Compila a ejecutables NXE nativos. No es una implementacion completa ni compatible de Java.

## Capacidades requeridas

| Area | Alcance que debemos construir | Estado |
|---|---|---|
| Tipos y metodos | Tipos primitivos, parametros, retornos, sobrecarga y conversiones definidas | Pendiente |
| Objetos | Instancias, campos, constructores, referencias, encapsulacion y acceso | Pendiente |
| Jerarquias | Herencia, interfaces, clases abstractas y despacho dinamico | Pendiente |
| Lenguaje avanzado | Arrays, enumeraciones, genericos, anotaciones, funciones lambda y demas construcciones de la referencia elegida | Pendiente |
| Errores | Excepciones, propagacion, captura y limpieza de recursos | Pendiente |
| Memoria | Heap de objetos y recoleccion de basura, con presupuesto para equipos de 1 GB | Pendiente |
| Concurrencia | Hilos reales, sincronizacion, monitores, espera/notificacion, atomicos, ejecutores y modelo de memoria definido | Pendiente |
| Bibliotecas | Cadenas, colecciones, matematicas, tiempo, entrada/salida, archivos, red y utilidades | Pendiente |
| Modulos y distribucion | Paquetes, importaciones, dependencias, carga de clases y herramientas de construccion | Pendiente |
| Compatibilidad Java | Seleccionar una version de referencia y verificar sintaxis, semantica, APIs y, por separado, formato de bytecode | Sin definir aun |
| Herramientas | Diagnosticos, depurador, pruebas, documentacion y asistencia del editor | Basico |

Esta tabla agrupa el objetivo; no es una lista exhaustiva de todas las especificaciones y APIs de Java. Se necesitara un inventario versionado antes de afirmar paridad. JVM, bibliotecas estandar y herramientas son partes distintas del alcance.

## Dependencias de NXK

Para los hilos del lenguaje, NXK necesita interrupciones, temporizador, planificador, pilas por tarea, cambios de contexto y primitivas de sincronizacion. Un bucle que alterna funciones no se presentara como equivalente a hilos de Java.

Para aplicaciones completas tambien faltan procesos aislados, memoria virtual, sistema de archivos y red. Las aplicaciones actuales corren con privilegios del kernel y no constituyen ese entorno.

## Secuencia de implementacion

1. Definir tipos, parametros y retornos; despues objetos y referencias.
2. Construir las bases de tareas y memoria del kernel en paralelo con el runtime del lenguaje.
3. Incorporar excepciones, gestion de memoria y bibliotecas fundamentales.
4. Implementar hilos y sincronizacion con semantica especificada y pruebas.
5. Ampliar bibliotecas y evaluar compatibilidad contra la version Java de referencia.
6. Introducir diferencias propias de NXJ de forma explicita y versionada.

El objetivo se conserva, pero no se promete que todas las aplicaciones Java funcionen actualmente, ni que sea posible replicar todo Java con unas pocas palabras adicionales.
