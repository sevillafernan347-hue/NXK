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
