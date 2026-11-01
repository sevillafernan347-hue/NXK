# Trabajar con tres cuentas

Git guarda el codigo y sus versiones. Cada chat necesita acceso al repositorio
o una copia adjunta; una cuenta de ChatGPT no recibe acceso automaticamente
por ser del mismo usuario. No compartas contrasenas ni tokens en el chat.

El repositorio es publico. Los colaboradores deben hacer un fork: una copia
independiente bajo su propia cuenta. Pueden guardar ahi todo el codigo que quieran
sin alterar el NXK original. Para proponer una pieza separada que podamos revisar,
usen `APORTES-EXTERNOS/<alias>/<componente>/` en su fork y abran un pull request
que solo agregue esa carpeta. `main` requiere pull request y no recibe cambios
directos. La cuenta principal revisa y decide si acepta el aporte; la integracion
posterior al kernel requiere otro cambio y aprobacion del usuario.

## Reparto inicial

| Responsable | Trabajo | Rama sugerida |
| --- | --- | --- |
| Principal | Arranque, mapa RAM, gestor de paginas e integracion | principal/memoria |
| Cuenta 2 | Ampliar NEXTRY: propuestas y funciones con parametros | lenguaje/parametros |
| Cuenta 3 | Revisar pruebas y documentacion; informar fallos encontrados | revision/calidad |

Cada sesion debe clonar en una carpeta distinta. Las dependencias de `tools`
se instalan en esa copia segun HERRAMIENTAS.md. No reutilizar la misma carpeta
de build de dos sesiones. La biblioteca queda fuera de Git.

## Protocolo de comunicacion

1. Leer el estado actual y anotar tarea, archivos previstos y commit base en el
   documento de entrega de la propia rama, antes de empezar.
2. Trabajar en los archivos de la tarea. Si hace falta cambiar una interfaz compartida,
   escribir una propuesta en la entrega y esperar que la principal la apruebe.
3. Entregar un commit y una solicitud de cambios (pull request), si la herramienta
   de la sesion tiene acceso de escritura. La principal debe revisar y probar antes de fusionar.
4. Si el chat solo puede leer o adjuntar archivos, entregar un parche o ZIP de cambios
   con su commit base. El usuario lo lleva a la cuenta principal.
5. Tras integrar, las otras sesiones actualizan desde main antes de la siguiente tarea.

El repositorio permite intercambiar resultados. No hay mensajeria automatica
entre estas cuentas configurada: cada asistente debe consultar las entregas
o el usuario debe trasladarlas. No asumir que otro chat vio un mensaje.

## Mensaje para pegar en la segunda cuenta

> Trabaja en el repositorio NXK que te comparto. No lo recrees. Lee AGENTS.md,
> ESTADO.md y COLABORACION.md. Tu tarea es proponer e implementar funciones
> con parametros en NEXTRY, empezando por revisar el compilador existente.
> Usa una copia separada y la rama lenguaje/parametros. No cambies boot, kernel,
> direcciones de memoria ni formatos ejecutables. Comenta en espanol y agrega
> pruebas utiles. Si una interfaz exige cambios de otra tarea, propone el contrato
> antes de editarlo. Entrega commit base, archivos cambiados, pruebas y limites.
> Si no tienes acceso al repositorio o herramientas, dilo antes de inventar resultados.

## Mensaje para pegar en la tercera cuenta

> Revisa el repositorio NXK que te comparto. Lee AGENTS.md, ESTADO.md y
> COLABORACION.md. Usa una copia separada y rama revision/calidad. Revisa las
> pruebas del ensamblador, los limites del cargador y la documentacion. Documenta
> fallos con un caso reproducible; no cambies arranque/kernel ni el compilador
> mientras las otras cuentas trabajan ahi. Puedes mejorar pruebas y documentos.
> Entrega commit base, hallazgos, cambios, pruebas ejecutadas y limites. No declares
> que el sistema esta completo ni que todos los archivos se escribieron en NEXTRY.

En ambos mensajes agrega el enlace real del repositorio. Si tu otra sesion no
puede abrir un repositorio privado, comparte una copia del codigo sin credenciales.
