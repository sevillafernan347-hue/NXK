/* Procesos CPL3 con cesion y preempcion: cuatro contextos CPU privados.
 * Puente C, una CPU, fuera de IRQ/tareas. El servicio INT81/9 cede el turno. */
#ifndef NXK_PROCESOS_H
#define NXK_PROCESOS_H
#include "usuario.h"
#include "espacios.h"
enum nxk_proceso_estado { NXK_PROCESO_LIBRE, NXK_PROCESO_LISTO,
    NXK_PROCESO_EJECUTANDO, NXK_PROCESO_TERMINADO, NXK_PROCESO_FALLIDO, NXK_PROCESO_DORMIDO,
    NXK_PROCESO_ESPERANDO_MENSAJE, NXK_PROCESO_CANCELADO,
    NXK_PROCESO_ESPERANDO_ENVIO, NXK_PROCESO_ESPERANDO_FIN };
#define NXK_PROCESO_ESTADOS (NXK_PROCESO_ESPERANDO_FIN+1u)
#define NXK_PROCESO_BUZON_MAX 4u
#define NXK_PROCESO_ARCHIVOS_MAX 4u
enum nxk_proceso_permiso {
    NXK_PERM_SALIDA=1u, NXK_PERM_LEER_ARCHIVOS=2u,
    NXK_PERM_MODIFICAR_ARCHIVOS=4u, NXK_PERM_ENVIAR_MENSAJES=8u,
    NXK_PERMISOS_TODOS=15u
};
struct nxk_proceso_mensaje { uint32_t origen,valor; };
struct nxk_proceso_resultado { uint32_t estado,codigo; };
struct nxk_proceso_info {
    uint32_t id,estado,turnos,cesiones,preempciones,error_motor,dormidas,despertar;
    /* mensajes: cantidad en cola. Los otros cuatro contadores saturan. */
    uint32_t mensajes,enviados,recibidos,esperas_mensaje,plazos;
    uint32_t esperando,esperas_envio,esperas_fin,ciclos;
    struct nxk_usuario_resultado ultimo;
    uint32_t permisos;
    uint32_t archivos_restringidos,archivos_permitidos;
};
/* Mascara propia de cada generacion, inicialmente TODOS. Consultar no resuelve
 * plazos; -1 ID/buffer/metadatos invalidos, salida intacta al rechazar.
 * La reduccion solo quita bits: 0 exito, -1 mascara/ID, -2 terminal,
 * -3 contexto kernel no permitido, -5 intento de recuperar permisos.
 * Variante actual reservada a INT81: solo el proceso EJECUTANDO con gestor
 * ocupado; no habilita privilegios nuevos ni actua sobre otro emisor. */
int nxk_proceso_permisos(uint32_t id,uint32_t *salida);
int nxk_proceso_restringir(uint32_t id,uint32_t mascara);
int nxk_proceso_restringir_actual(uint32_t id,uint32_t mascara);
/* Lista de nombres exactos de1..23 caracteres RAMFS (a-z,0-9,_,-,.).
 * Sin filtro al crear. La primera lista activa el filtro; despues solo se
 * aceptan subconjuntos, sin duplicados. cantidad0 admite nombres null y niega
 * todos. Entradas kernel confiables; se copian antes de modificar la politica.
 * 0 exito, -1 contrato/ID/metadatos, -2 terminal/ejecutando, -3 contexto,
 * -5 intento de ampliar. La variante actual solo en el turno EJECUTANDO.
 * Cancelar conserva la lista; una generacion nueva comienza sin filtro. */
int nxk_proceso_limitar_archivos(uint32_t id,const char nombres[][24],uint32_t cantidad);
int nxk_proceso_limitar_archivos_actual(uint32_t id,const char nombres[][24],uint32_t cantidad);
/* Solo el filtro de nombres, independiente de permisos generales:1 permite,
 * 0 deniega,-1 ID/nombre/metadatos. No resuelve plazos ni consulta archivos. */
int nxk_proceso_archivo_permitido(uint32_t id,const char *nombre);
struct nxk_procesos_resumen {
    uint32_t registrados,pendientes,finalizados,estados[NXK_PROCESO_ESTADOS];
    uint32_t mensajes,referencias_fin;
};
/* Instantanea de estados almacenados, sin despertar, depurar ni asignar heap.
 * LIBRE cuenta slots sin PCB vivo, no garantiza espacio para crear: un espacio
 * sin proceso tambien puede ocupar el pool. referencias_fin cuenta observadores,
 * no destinos distintos, incluso si estan listos tras vencer su plazo.
 * Solo kernel original/IF, fuera de tarea y ejecutor. -1 null, -2 metadatos,
 * -3 contexto. Cualquier error conserva la salida. Omite IDs obsoletos. */
int nxk_procesos_resumen(struct nxk_procesos_resumen *salida);
/* Crear/cargar conserva salida al fallar. IDs son los IDs completos del espacio.
 * Rechazos: -1 contrato/ID, -2 estado/pool, -3 contexto; memoria/motor negativos. */
int nxk_proceso_crear(const void *codigo,size_t longitud,uint32_t *id);
int nxk_proceso_cargar_archivo(const char *nombre,uint32_t *id);
int nxk_proceso_consultar(uint32_t id,struct nxk_proceso_info *salida);
/* -2 si el slot esta libre. Errores nunca modifican salida. */
int nxk_proceso_enumerar(unsigned slot,struct nxk_proceso_info *salida);
int nxk_proceso_paso(uint32_t id);
/* FIFO de cuatro mensajes, origen0 para el kernel. No bloquea ni descarta al
 * llenar: 0 exito, -1 ID, -2 lleno, -3 destino terminal o contexto kernel.
 * Solo fuera del ejecutor, con las mismas condiciones que crear/paso. */
int nxk_proceso_enviar(uint32_t destino,uint32_t valor);
/* Puentes internos de INT81: origen/id debe ser el proceso en EJECUTANDO y
 * el gestor debe estar ocupado. Un error no consume ni modifica salida.
 * Enviar exige ENVIAR_MENSAJES; -5 si se ha revocado.
 * Recibir: -2 vacio, -1 contrato. Preparar conserva el buffer virtual hasta
 * reanudar; el gestor entrega {origen,valor} antes de retirar de la cola.
 * Buffer de ocho bytes en pila/datos reservados; un cruce retiene ambas
 * paginas de datos hasta consumir, cancelar o retirar la espera. */
int nxk_proceso_enviar_desde(uint32_t origen,uint32_t destino,uint32_t valor);
int nxk_proceso_recibir_actual(uint32_t id,struct nxk_proceso_mensaje *salida);
int nxk_proceso_preparar_espera(uint32_t id,uint32_t buffer_virtual);
/* Puentes INT81/14..15: solo proceso actual. 0 exito, 1 consulta/plazo,
 * 2 preparar suspension, -1 contrato/ID, -3 envio a terminal, -4 ciclo,
 * -5 envio sin permiso (tambien si se revoca durante la espera).
 * El motor guarda marco y plazo absoluto despues de un retorno2. No reservar
 * sitio en la cola al esperar; el siguiente despacho vuelve a comprobarlo. */
int nxk_proceso_enviar_esperando_actual(uint32_t id,uint32_t destino,uint32_t valor,uint32_t ticks);
int nxk_proceso_esperar_fin_actual(uint32_t id,uint32_t destino,uint32_t buffer_virtual,uint32_t ticks,struct nxk_proceso_resultado *salida);
/* Referencia de resultado pendiente, incluso listo tras vencer. Protege la
 * liberacion del destino hasta entregar, cancelar o retirar al observador.
 * Solo consulta metadatos y generaciones; no requiere el CR3 del kernel. */
int nxk_proceso_retenido(uint32_t id);
/* Retencion de una pagina virtual alineada por el buffer8 de recepcion/FIN
 * de este proceso vivo. Incluye LISTO tras plazo hasta consumir su espera;
 * no resuelve plazos. Cancelar/entregar/liberar retira la retencion. */
int nxk_proceso_pagina_retenida(uint32_t id,uint32_t pagina);
/* Cancelar LISTO/DORMIDO o cualquier espera: limpia buzon y espera, retiene ID y
 * memoria hasta liberar. -1 ID, -2 estado terminal, -3 contexto/gestor ocupado. */
int nxk_proceso_cancelar(uint32_t id);
/* Hasta1024 despachos: 0 sin pendientes, 1 presupuesto, 2 solo dormidos/esperas.
 * Un fallo CPU de una app no impide ejecutar otras. Error del motor negativo. */
int nxk_procesos_ejecutar(uint32_t presupuesto);
/* -5 mientras otra espera FIN conserve una referencia al resultado. */
int nxk_proceso_liberar(uint32_t id);
/* Impide arrancar por la ruta legacy un espacio con contexto CPU conservado. */
int nxk_proceso_gestionado(uint32_t id);
int nxk_procesos_demo(void);
int nxk_dependencias_demo(void);
int nxk_preempcion_demo(void);
int nxk_proceso_dormir_demo(void);
#ifdef NXK_TEST
int nxk_procesos_probar(void);
int nxk_dependencias_probar(void);
int nxk_preempcion_probar(void);
int nxk_proceso_dormir_probar(void);
#endif
#endif
