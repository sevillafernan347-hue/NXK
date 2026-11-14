# Una tarea separada para Gemini

Puedes compartir el repositorio publico y pegar este encargo en Gemini.
El intercambio es mediante Git o un parche que traes a este chat; no hay
comunicacion automatica entre las cuentas o modelos.

> Revisa NXK desde la etiqueta v0.0.29 del repositorio
> https://github.com/sevillafernan347-hue/NXK . Lee AGENTS.md, ESTADO.md,
> COLABORACION.md y PENDIENTES-KERNEL.md. Usa tu propio fork o copia y rama
> revision/gemini-ata. Tu tarea es revisar kernel/ata.c y su cabecera:
> limites de sectores, desbordamientos, timeout, errores del dispositivo y
> conservacion de buffers al rechazar entradas. No cambies el gestor de memoria,
> procesos, direcciones, ABI, lenguaje ni interfaz. No copies codigo de Linux.
> Entrega hallazgos concretos con archivo/linea, desencadenante y evidencia;
> distingue fallos confirmados de dudas. Puedes proponer pruebas reproducibles
> y cambios en APORTES-EXTERNOS/gemini/ata/ de tu copia. No los integres en main.
> Usa solo discos virtuales desechables para cualquier prueba de escritura.
> Incluye el commit base exacto, archivos cambiados, comandos ejecutados,
> resultados reales y limitaciones. Si no puedes ejecutar, dilo expresamente.
> Entrega un commit o parche; la integracion principal revisara y probara antes
> de aceptar. No solicites tokens ni contrasenas.

Para traer el aporte basta el archivo de parche o el enlace al commit/PR del
fork. No es necesario conectar cuentas ni dar permisos de escritura al original.
