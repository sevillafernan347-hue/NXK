# Copias de archivos visibles a RAM

Base: 20def18. Integracion realizada por la cuenta principal.

Se incorpora nxk_archivo_copiar en el puente C de archivos y el comando
`copiar origen destino`. Se puede copiar un archivo RAM o un archivo del
volumen NXFS montado a una entrada RAM nueva, editable con `agregar`.
Ejemplo:

```
volumen montar
copiar leeme.txt editable
agregar editable texto_local
leer editable
```

El origen se lee completo y verifica antes de publicar el destino. La copia
admite hasta 256 bytes, incluyendo archivos vacios. Un destino visible existente
se conserva, incluso si esta en NXFS, supera 256 bytes o falla su verificacion.
Solo el resultado de ausencia permite crear la copia. No reemplaza ni trunca.
Si faltan entradas RAM, el origen y los archivos existentes permanecen intactos.
La operacion no escribe en disco y las copias se pierden al apagar.

Pruebas ejecutadas:
- Autopruebas de kernel: tabla llena, 256 bytes, igualdad de contenido, destino
  ocupado, mismo nombre, origen ausente, nombre invalido, independencia y vacio.
- Matriz x86 QEMU: 32, 64, 128 y 1024 MiB.
- Nueve escenarios NXFS de pruebas_volumen.py, ampliados con copia desde disco,
  edicion de copia, colision RAM y disco, rechazo de origen de 1100 bytes,
  copia vacia, copia de 256 bytes, ausencia y contenido corrupto sin publicacion.
  Las imagenes base permanecieron iguales byte a byte.
- Regresion x64: arranque y autopruebas con 64 MiB.

No cambia NXE ni INT 0x81. Todavia no se expone como instruccion NEXTRY.
La comprobacion y publicacion asumen el modelo actual sin concurrencia;
se necesitara sincronizacion al incorporar procesos concurrentes.
