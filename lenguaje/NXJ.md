# NXJ 0.1

El objetivo del proyecto es ampliar NXJ hasta ofrecer las capacidades y comodidad de Java antes de introducir divergencias propias. Hilos, objetos, bibliotecas y demas requisitos se registran en `OBJETIVO-NXJ.md`; la version actual todavia no los implementa.

Variante propia de NXK Espanol inspirada en la organizacion por clases de Java. Usa fuentes `.nxj` y produce ejecutables nativos `.nxe`. No copia el codigo de Java y no implementa su JVM ni sus bibliotecas.

```text
clase MiAplicacion
    publico estatico funcion inicio
        mostrar "Hola desde NXJ"
        variable contador = 0
        repetir 3 veces
            contador = contador + 1
        fin
        numero contador
    fin
fin
```

## Reglas implementadas

- Una clase por archivo, cerrada con `fin`.
- Metodos declarados como `publico estatico funcion nombre` y cerrados con `fin`.
- Un metodo `inicio` obligatorio, que sirve como punto de entrada.
- Los metodos pueden llamar a otros con `llamar nombre`.
- Dentro de cada metodo se aplican las reglas, tipos y operaciones de NXK Espanol 0.1.
- Una orden por linea; la sangria es orientativa.
- `clase`, `publico` y `estatico` son las nuevas palabras de esta variante.

En esta primera version la clase organiza los metodos: no se crean instancias. Los metodos son publicos y estaticos, sin parametros ni valores de retorno. Objetos, constructores, campos, herencia, interfaces, excepciones, recoleccion de basura y compatibilidad con fuentes Java o archivos `.class` no estan implementados.

## Construir

Ejecuta `CREAR-APP-NXJ.cmd`. El resultado es `build/x86/hola-java.nxe`. La construccion usa el mismo runtime y formato NXE que las fuentes `.nxf`.

El shell de NXK sigue exponiendo solo `hola.nxe` y `hola.exe` embebidos. Crear este archivo no lo agrega automaticamente a ese menu ni permite abrir rutas arbitrarias. Para probar su codigo en la imagen actual se puede incorporar el ejemplo como fuente de la aplicacion embebida durante la construccion.

Esta es una variante inicial funcional, no un reemplazo completo de Java. La siguiente ampliacion natural son parametros y retornos tipados; despues, objetos y campos.
