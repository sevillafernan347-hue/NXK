# Contribuir a NXK

NXK es un prototipo educativo de kernel original para x86. Queremos aportes
pequenos, explicados y verificables. El objetivo actual es BIOS x86 de 32 bits
y maquinas con poca RAM. Lee primero `README.md`, `ESTADO.md`, `AGENTS.md` y
`PLAN-8-SESIONES.md`.

## Proponer un cambio

1. Abre un issue describiendo el problema, el equipo/entorno y como reproducirlo.
2. Haz fork del repositorio y crea una rama para un solo cambio. Tu fork es tu copia
   independiente; trabajar ahi no modifica el proyecto original.
3. Para compartir un componente opcional sin editar NXK, agrega una carpeta nueva
   en `APORTES-EXTERNOS/<tu-alias>/<nombre-del-componente>/`. Incluye su codigo,
   instrucciones y pruebas propias. No reemplaces archivos de NXK ni uses sus rutas
   para guardar variantes experimentales.
4. Abre un pull request que solo agregue tu carpeta a `APORTES-EXTERNOS/`.
   Esto conserva tu version aparte del kernel principal. El mantenedor decidira si
   la acepta como aporte independiente; integrar codigo en el kernel requiere una
   propuesta separada y aprobacion explicita.
5. No regeneres NXK desde cero. Conserva comentarios en espanol y explica cualquier
   puente temporal a C, ensamblador externo o herramienta de compilacion.
6. Incluye pruebas. Para cambios del kernel ejecuta `test.ps1 -Architecture x86`;
   si toca RAM, ejecuta tambien `test.ps1 -Architecture x86 -MemoryMatrix`.
   Para cambios del lenguaje, ejecuta `lenguaje/pruebas.py`.
7. Abre un pull request con commit base, archivos tocados, resultados reales y
   limitaciones conocidas. No digas que probaste hardware fisico si no fue asi.

## Limites del proyecto

No enviar contrasenas, tokens, libros con derechos ajenos, herramientas, imagenes
de disco ni datos personales. No cambiar direcciones de memoria, ABI, formato NXE
o protocolos compartidos sin documentar la propuesta. No se aceptan afirmaciones
de compatibilidad general con Windows sin implementacion y pruebas.

El codigo actual no declara una licencia de redistribucion. La visibilidad publica
permite leer y descargarlo en GitHub; no concede por si sola una licencia para
publicar derivados fuera de lo permitido por la plataforma. Pregunta al mantenedor
antes de basar una distribucion externa en el codigo. Las dependencias conservan
sus propias licencias.
