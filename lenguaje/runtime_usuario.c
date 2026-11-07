/* Runtime propio CPL3. Salida por INT 0x81; finalizacion por INT 0x80.
 * Sin puertos ni informacion del heap kernel. No usa punteros de API ring0. */
#include "nxk_es.h"
static uint32_t escribir(const void *p,uint32_t n) {
    uint32_t r=1;
    __asm__ volatile("int $0x81":"+a"(r):"b"(p),"c"(n):"memory","cc");
    return r;
}
static _Noreturn void terminar(uint32_t n) {
    __asm__ volatile("int $0x80" :: "a"(n) : "memory");
    for (;;) __asm__ volatile("ud2");
}
_Noreturn void nxk_es_fallo(uint32_t linea) {
    (void)linea; const char *t="Error de aplicacion NEXTRY\n";
    escribir(t,sizeof("Error de aplicacion NEXTRY\n")-1); terminar(1);
}
/* Solo el motor cooperativo atiende servicio9. Una ruta sin reanudacion
 * devuelve error: no fingir que la cesion ocurrio ni continuar silenciosamente. */
void nxk_es_ceder(void) {
    uint32_t r=9;
    __asm__ volatile("int $0x81":"+a"(r)::"memory","cc");
    if (r) nxk_es_fallo(0);
}
/* Servicio10 conserva el contexto hasta vencer el plazo. Cero cede sin
 * dormir; el kernel rechaza intervalos mayores que 0x7fffffff ticks. */
void nxk_es_dormir(uint32_t ticks) {
    uint32_t r=10;
    __asm__ volatile("int $0x81":"+a"(r):"b"(ticks):"memory","cc");
    if (r) nxk_es_fallo(0);
}
/* Servicios11..13 son exclusivos del gestor de procesos. El kernel registra
 * el emisor real, comprueba el destino y limita cada cola a cuatro mensajes. */
void nxk_es_enviar_mensaje(uint32_t destino,uint32_t valor) {
    uint32_t r=11;
    __asm__ volatile("int $0x81":"+a"(r):"b"(destino),"c"(valor):"memory","cc");
    if (r) nxk_es_fallo(0);
}
uint32_t nxk_es_recibir_mensaje(uint32_t *origen,uint32_t *valor,uint32_t ticks) {
    uint32_t r=12, mensaje[2];
    if (ticks>UINT32_C(0x7fffffff)) nxk_es_fallo(0);
    /* El registro temporal esta en la pila privada. Solo publicamos salidas
     * despues de que el kernel confirme una entrega completa de ocho bytes. */
    __asm__ volatile("int $0x81":"+a"(r):"b"(mensaje),"d"(ticks):"memory","cc");
    if (!r) { *origen=mensaje[0]; *valor=mensaje[1]; }
    else if (r!=1) nxk_es_fallo(0);
    return r;
}
uint32_t nxk_es_proceso_actual(void) {
    uint32_t r=13;
    __asm__ volatile("int $0x81":"+a"(r)::"memory","cc");
    if (!r || r>=UINT32_C(0xfffffffe)) nxk_es_fallo(0);
    return r;
}
void nxk_es_mostrar(const char *t) {
    uint32_t n=0; while (n<256 && t[n]) ++n;
    if (n==256 || escribir(t,n)!=n || escribir("\n",1)!=1) nxk_es_fallo(0);
}
void nxk_es_numero(uint32_t n) {
    char b[10]; uint32_t i=10;
    do { b[--i]=(char)('0'+n%10); n/=10; } while (n);
    if (escribir(b+i,10-i)!=10-i || escribir("\n",1)!=1) nxk_es_fallo(0);
}
static uint32_t archivo(uint32_t servicio,const char *nombre,void *datos,uint32_t n) {
    __asm__ volatile("int $0x81":"+a"(servicio):"b"(nombre),"c"(datos),"d"(n):"memory","cc");
    return servicio;
}
void nxk_es_guardar_archivo(const char *nombre,const char *texto) {
    uint32_t n=0; while (n<256 && texto[n]) ++n;
    if (n==256 || archivo(2,nombre,(void *)texto,n)) nxk_es_fallo(0);
}
void nxk_es_agregar_archivo(const char *nombre,const char *texto) {
    uint32_t n=0; while (n<256 && texto[n]) ++n;
    if (n==256 || archivo(6,nombre,(void *)texto,n)) nxk_es_fallo(0);
}
void nxk_es_copiar_archivo(const char *origen,const char *destino) {
    if (archivo(8,origen,(void *)destino,0)) nxk_es_fallo(0);
}
void nxk_es_renombrar_archivo(const char *nombre,const char *nuevo) {
    if (archivo(7,nombre,(void *)nuevo,0)) nxk_es_fallo(0);
}
void nxk_es_mostrar_archivo(const char *nombre) {
    char datos[256]; uint32_t posicion=0;
    for (;;) {
        uint32_t n=5;
        __asm__ volatile("int $0x81":"+a"(n):"b"(nombre),"c"(datos),"d"(256u),"S"(posicion):"memory","cc");
        if (n>256) nxk_es_fallo(0);
        if (!n) break;
        if (escribir(datos,n)!=n) nxk_es_fallo(0);
        posicion+=n;
        if (posicion>4096) nxk_es_fallo(0);
    }
    if (escribir("\n",1)!=1) nxk_es_fallo(0);
}
void nxk_es_borrar_archivo(const char *nombre) {
    if (archivo(4,nombre,0,0)) nxk_es_fallo(0);
}
uint32_t nxk_es_dividir(uint32_t a,uint32_t b) { if (!b) nxk_es_fallo(0); return a/b; }
uint32_t nxk_es_resto(uint32_t a,uint32_t b) { if (!b) nxk_es_fallo(0); return a%b; }
uint32_t nxk_es_memoria_usada(void) { nxk_es_fallo(0); }
uint32_t nxk_es_leer_puerto(uint32_t p) { (void)p; nxk_es_fallo(0); }
void nxk_es_escribir_puerto(uint32_t p,uint32_t v) { (void)p;(void)v; nxk_es_fallo(0); }
_Noreturn void nxk_es_detener(void) { terminar(0); }
__attribute__((naked,section(".text.entry"))) void usuario_entrada(void) {
    __asm__ volatile("call nxk_es_inicio; xor %eax,%eax; int $0x80; ud2");
}
