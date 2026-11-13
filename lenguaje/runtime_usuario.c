/* Runtime propio CPL3. Salida por INT 0x81; finalizacion por INT 0x80.
 * Sin puertos ni informacion del heap kernel. No usa punteros de API ring0. */
#include "nxk_es.h"
static uint32_t escribir(const void *p,uint32_t n) {
    uint32_t r=1;
    __asm__ volatile("int $0x81":"+a"(r):"b"(p),"c"(n):"memory","cc");
    return r;
}
/* Estos intrinsecos permiten inspeccionar errores del servicio sin abortar.
 * Un kernel anterior que no ofrece el servicio si termina explicitamente;
 * no se debe presentar ese caso como un plazo vencido o una lectura vacia. */
static uint32_t resultado_buffer(uint32_t resultado) {
    if (resultado==UINT32_C(0xfffffffe)) nxk_es_fallo(0);
    return resultado;
}
uint32_t nxk_es_mostrar_memoria(uint32_t direccion,uint32_t cantidad) {
    return resultado_buffer(escribir((const void *)(uintptr_t)direccion,cantidad));
}
uint32_t nxk_es_leer_archivo_en(const char *nombre,uint32_t direccion,uint32_t cantidad,uint32_t desplazamiento) {
    uint32_t r=5;
    __asm__ volatile("int $0x81":"+a"(r):"b"(nombre),"c"(direccion),"d"(cantidad),"S"(desplazamiento):"memory","cc");
    return resultado_buffer(r);
}
uint32_t nxk_es_guardar_archivo_desde(const char *nombre,uint32_t direccion,uint32_t cantidad) {
    uint32_t r=2;
    __asm__ volatile("int $0x81":"+a"(r):"b"(nombre),"c"(direccion),"d"(cantidad):"memory","cc");
    return resultado_buffer(r);
}
uint32_t nxk_es_recibir_en_memoria(uint32_t direccion,uint32_t ticks) {
    uint32_t r=12;
    __asm__ volatile("int $0x81":"+a"(r):"b"(direccion),"d"(ticks):"memory","cc");
    return resultado_buffer(r);
}
uint32_t nxk_es_esperar_en_memoria(uint32_t destino,uint32_t direccion,uint32_t ticks) {
    uint32_t r=15;
    __asm__ volatile("int $0x81":"+a"(r):"b"(destino),"c"(direccion),"d"(ticks):"memory","cc");
    return resultado_buffer(r);
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
/* Servicio16 reduce permisos del proceso actual; incluso la mascara0 es
 * valida. No confundir un permiso denegado con una restriccion aplicada. */
void nxk_es_restringir_permisos(uint32_t mascara) {
    uint32_t r=16;
    __asm__ volatile("int $0x81":"+a"(r):"b"(mascara):"memory","cc");
    if (r) nxk_es_fallo(0);
}
uint32_t nxk_es_permisos_actuales(void) {
    uint32_t r=17;
    __asm__ volatile("int $0x81":"+a"(r)::"memory","cc");
    /* Todos los cuatro bits, y ningun otro. Un servicio no disponible o un
     * error negativo tampoco debe parecer una mascara de permisos valida. */
    if (r&~UINT32_C(15)) nxk_es_fallo(0);
    return r;
}
void nxk_es_limitar_memoria(uint32_t cuota) {
    uint32_t r=21;
    __asm__ volatile("int $0x81":"+a"(r):"b"(cuota):"memory","cc");
    if (r) nxk_es_fallo(0);
}
uint32_t nxk_es_cuota_memoria(void) {
    uint32_t r=22;
    __asm__ volatile("int $0x81":"+a"(r)::"memory","cc");
    if (r>4) nxk_es_fallo(0);
    return r;
}
/* La tabla se copia a la pila propia; no prestar cadenas de la app al PCB.
 * El kernel vuelve a validar cada nombre y aplica solo reducciones de lista. */
void nxk_es_limitar_archivos(uint32_t cantidad,const char *a,const char *b,const char *c,const char *d) {
    if (cantidad>4) nxk_es_fallo(0);
    const char *entradas[4]={a,b,c,d};
    char nombres[4][24];
    /* Volatile evita introducir una dependencia externa de memset en CPL3. */
    volatile unsigned char *limpiar=(volatile unsigned char *)nombres;
    for (unsigned i=0;i<sizeof(nombres);++i) limpiar[i]=0;
    for (unsigned i=0;i<cantidad;++i) {
        const char *nombre=entradas[i]; if (!nombre) nxk_es_fallo(0);
        unsigned j;
        for (j=0;j<24;++j) { nombres[i][j]=nombre[j]; if (!nombre[j]) break; }
        if (!j || j==24) nxk_es_fallo(0);
    }
    uint32_t resultado=20;
    const void *tabla=cantidad ? (const void *)nombres : 0;
    __asm__ volatile("int $0x81":"+a"(resultado):"b"(tabla),"c"(cantidad):"memory","cc");
    if (resultado) nxk_es_fallo(0);
}
/* Servicios18/19 administran solo las paginas privadas del proceso actual.
 * Cero de reserva y error de liberacion son resultados manejables por la app;
 * un servicio ausente no debe confundirse con memoria agotada. */
uint32_t nxk_es_reservar_pagina(void) {
    uint32_t r=18;
    __asm__ volatile("int $0x81":"+a"(r)::"memory","cc");
    if (r && (r<UINT32_C(0x402000) || r>=UINT32_C(0x406000) || (r&4095u))) nxk_es_fallo(0);
    return r;
}
uint32_t nxk_es_reservar_paginas(uint32_t cantidad) {
    uint32_t r=23;
    __asm__ volatile("int $0x81":"+a"(r):"b"(cantidad):"memory","cc");
    if (r && (!cantidad || cantidad>4 || (r&4095u) || r<0x402000u ||
        r>0x406000u-cantidad*4096u)) nxk_es_fallo(0);
    return r;
}
uint32_t nxk_es_liberar_paginas(uint32_t direccion,uint32_t cantidad) {
    uint32_t r=24;
    __asm__ volatile("int $0x81":"+a"(r):"b"(direccion),"c"(cantidad):"memory","cc");
    if (r && r!=UINT32_MAX && r!=(uint32_t)-5) nxk_es_fallo(0);
    return r;
}
uint32_t nxk_es_liberar_pagina(uint32_t direccion) {
    uint32_t r=19;
    __asm__ volatile("int $0x81":"+a"(r):"b"(direccion):"memory","cc");
    if (r && r!=UINT32_MAX) nxk_es_fallo(0);
    return r;
}
/* El acceso ocurre en CPL3, nunca mediante un puntero confiado al kernel.
 * Se comprueba alineacion e intervalo completo. Si ya fue liberada, la CPU
 * provoca fallo de pagina que termina solo esta aplicacion. */
static void comprobar_entero(uint32_t direccion) {
    if (direccion<UINT32_C(0x402000) || direccion>UINT32_C(0x405ffc) || (direccion&3u)) nxk_es_fallo(0);
}
uint32_t nxk_es_leer_entero(uint32_t direccion) {
    comprobar_entero(direccion);
    return *(volatile const uint32_t *)(uintptr_t)direccion;
}
void nxk_es_escribir_entero(uint32_t direccion,uint32_t valor) {
    comprobar_entero(direccion);
    *(volatile uint32_t *)(uintptr_t)direccion=valor;
}
/* Enviar con plazo conserva el valor en el PCB si la cola esta llena.
 * Retornos negativos (incluido ciclo) no se confunden con vencimiento. */
uint32_t nxk_es_enviar_esperando(uint32_t destino,uint32_t valor,uint32_t ticks) {
    uint32_t r=14;
    if (ticks>UINT32_C(0x7fffffff)) nxk_es_fallo(0);
    __asm__ volatile("int $0x81":"+a"(r):"b"(destino),"c"(valor),"d"(ticks):"memory","cc");
    if (r>1) nxk_es_fallo(0);
    return r;
}
uint32_t nxk_es_esperar_proceso(uint32_t destino,uint32_t *estado,uint32_t *codigo,uint32_t ticks) {
    uint32_t r=15, resultado[2];
    if (ticks>UINT32_C(0x7fffffff)) nxk_es_fallo(0);
    /* El servicio escribe su registro en la pila privada. Una consulta vacia
     * o plazo vencido no publica bytes no inicializados sobre las variables. */
    __asm__ volatile("int $0x81":"+a"(r):"b"(destino),"c"(resultado),"d"(ticks):"memory","cc");
    if (!r) { *estado=resultado[0]; *codigo=resultado[1]; }
    else if (r!=1) nxk_es_fallo(0);
    return r;
}
_Noreturn void nxk_es_finalizar(uint32_t codigo) { terminar(codigo); }
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
