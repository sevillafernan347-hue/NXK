/* GUIA DE LECTURA NXK - lenguaje/runtime_nxe.c
 * Adaptador de la app NXE: recibe la tabla de servicios del kernel y llama al inicio de NEXTRY.
 * Contexto y orden de estudio: ver LEER-CODIGO.md en la carpeta principal.
 */
#include "nxe.h"
#include "nxk_es.h"
static const NXE_API *sistema;
void nxk_es_mostrar(const char *t) { sistema->mostrar(t); }
void nxk_es_numero(uint32_t n) { sistema->numero(n); }
uint32_t nxk_es_memoria_usada(void) { return sistema->memoria_usada(); }
uint32_t nxk_es_dividir(uint32_t a, uint32_t b) { return sistema->dividir(a, b); }
uint32_t nxk_es_resto(uint32_t a, uint32_t b) { return sistema->resto(a, b); }
uint32_t nxk_es_leer_puerto(uint32_t p) { return sistema->leer_puerto(p); }
void nxk_es_escribir_puerto(uint32_t p, uint32_t v) { sistema->escribir_puerto(p, v); }
_Noreturn void nxk_es_fallo(uint32_t n) {
    sistema->fallo(n);
    for (;;) __asm__ volatile("cli; hlt");
}
_Noreturn void nxk_es_detener(void) {
    sistema->detener();
    for (;;) __asm__ volatile("cli; hlt");
}
__attribute__((section(".text.entry")))
uint32_t nxe_entrada(const NXE_API *api) {
    if (!api || api->version != 1 || api->longitud != sizeof(NXE_API)) return 1;
    sistema = api;
    nxk_es_inicio();
    return 0;
}

/* ABI NXE 1 no publica archivos; no cambiarla silenciosamente. */
void nxk_es_guardar_archivo(const char *n,const char *t) { (void)n;(void)t; nxk_es_fallo(0); }
void nxk_es_mostrar_archivo(const char *n) { (void)n; nxk_es_fallo(0); }
void nxk_es_borrar_archivo(const char *n) { (void)n; nxk_es_fallo(0); }

void nxk_es_agregar_archivo(const char *n,const char *t) { (void)n;(void)t;nxk_es_fallo(0); }
void nxk_es_copiar_archivo(const char *origen,const char *destino) {
    (void)origen; (void)destino; nxk_es_fallo(0);
}
void nxk_es_renombrar_archivo(const char *n,const char *t) { (void)n;(void)t;nxk_es_fallo(0); }

/* ABI NXE1 no publica reanudacion cooperativa. */
void nxk_es_ceder(void) { nxk_es_fallo(0); }

/* ABI NXE1 no publica espera con contexto de proceso. */
void nxk_es_dormir(uint32_t ticks) { (void)ticks; nxk_es_fallo(0); }

/* ABI NXE1 no ofrece identidad ni buzon de proceso. No simular IPC en ring0. */
void nxk_es_enviar_mensaje(uint32_t destino,uint32_t valor) {
    (void)destino; (void)valor; nxk_es_fallo(0);
}
uint32_t nxk_es_recibir_mensaje(uint32_t *origen,uint32_t *valor,uint32_t ticks) {
    (void)origen; (void)valor; (void)ticks; nxk_es_fallo(0);
}
uint32_t nxk_es_proceso_actual(void) { nxk_es_fallo(0); }

/* ABI NXE1 no tiene espera por proceso ni salida no local con codigo.
 * No detener toda la maquina fingiendo que finalizo solo esta aplicacion. */
uint32_t nxk_es_enviar_esperando(uint32_t destino,uint32_t valor,uint32_t ticks) {
    (void)destino; (void)valor; (void)ticks; nxk_es_fallo(0);
}
uint32_t nxk_es_esperar_proceso(uint32_t destino,uint32_t *estado,uint32_t *codigo,uint32_t ticks) {
    (void)destino; (void)estado; (void)codigo; (void)ticks; nxk_es_fallo(0);
}
_Noreturn void nxk_es_finalizar(uint32_t codigo) { (void)codigo; nxk_es_fallo(0); }

/* NXE1 se ejecuta en ring0 y no ofrece permisos por proceso. No presentar
 * una reduccion ficticia como si protegiera archivos o llamadas del kernel. */
void nxk_es_restringir_permisos(uint32_t mascara) { (void)mascara; nxk_es_fallo(0); }
uint32_t nxk_es_permisos_actuales(void) { nxk_es_fallo(0); }
void nxk_es_limitar_memoria(uint32_t cuota) { (void)cuota; nxk_es_fallo(0); }
uint32_t nxk_es_cuota_memoria(void) { nxk_es_fallo(0); }
/* NXE1 no incorpora una identidad de proceso con lista de archivos propia. */
void nxk_es_limitar_archivos(uint32_t cantidad,const char *a,const char *b,const char *c,const char *d) {
    (void)cantidad; (void)a; (void)b; (void)c; (void)d; nxk_es_fallo(0);
}

/* NXE1 no tiene paginas privadas de proceso. No acceder a estas direcciones
 * desde ring0 ni simular que una reserva o liberacion tuvo exito. */
uint32_t nxk_es_reservar_pagina(void) { nxk_es_fallo(0); }
void nxk_es_limitar_destinos(uint32_t n,uint32_t a,uint32_t b,uint32_t c,uint32_t d) {
    (void)n;(void)a;(void)b;(void)c;(void)d;nxk_es_fallo(0);
}
uint32_t nxk_es_reservar_paginas(uint32_t n) { (void)n; nxk_es_fallo(0); }
uint32_t nxk_es_liberar_paginas(uint32_t p,uint32_t n) { (void)p;(void)n; nxk_es_fallo(0); }
uint32_t nxk_es_proteger_paginas(uint32_t p,uint32_t n,uint32_t e) { (void)p;(void)n;(void)e; nxk_es_fallo(0); }
uint32_t nxk_es_escritura_pagina(uint32_t p) { (void)p; nxk_es_fallo(0); }
uint32_t nxk_es_liberar_pagina(uint32_t direccion) { (void)direccion; nxk_es_fallo(0); }
uint32_t nxk_es_leer_entero(uint32_t direccion) { (void)direccion; nxk_es_fallo(0); }
void nxk_es_escribir_entero(uint32_t direccion,uint32_t valor) {
    (void)direccion; (void)valor; nxk_es_fallo(0);
}
/* NXE1 no dispone de buffers CPL3: no confiar direcciones de la app a ring0. */
uint32_t nxk_es_leer_archivo_en(const char *nombre,uint32_t direccion,uint32_t cantidad,uint32_t desplazamiento) {
    (void)nombre; (void)direccion; (void)cantidad; (void)desplazamiento; nxk_es_fallo(0);
}
uint32_t nxk_es_guardar_archivo_desde(const char *nombre,uint32_t direccion,uint32_t cantidad) {
    (void)nombre; (void)direccion; (void)cantidad; nxk_es_fallo(0);
}
uint32_t nxk_es_mostrar_memoria(uint32_t direccion,uint32_t cantidad) {
    (void)direccion; (void)cantidad; nxk_es_fallo(0);
}
uint32_t nxk_es_recibir_en_memoria(uint32_t direccion,uint32_t ticks) {
    (void)direccion; (void)ticks; nxk_es_fallo(0);
}
uint32_t nxk_es_esperar_en_memoria(uint32_t destino,uint32_t direccion,uint32_t ticks) {
    (void)destino; (void)direccion; (void)ticks; nxk_es_fallo(0);
}
