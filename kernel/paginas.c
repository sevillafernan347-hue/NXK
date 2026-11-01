/* Gestor inicial de paginas fisicas. Puente C pendiente de migrar a NEXTRY.
 * Reserva los primeros 8 MiB: BIOS, kernel, pilas, heap, NXE, PE32 y metadatos.
 * Dos mapas de bits distinguen paginas libres de paginas entregadas.
 * Las regiones reservadas del BIOS prevalecen incluso si se solapan con RAM.
 * Todavia no es seguro para interrupciones concurrentes ni multiples CPU.
 */
#include "paginas.h"
#define LIMITE 0x40000000u
#define PRIMERA (0x800000u/4096)
#define CANTIDAD (LIMITE/4096)
#define BYTES (CANTIDAD/8)
static uint32_t libres, total;
#if defined(__i386__)
static uint32_t pista=PRIMERA, entregas_activas;
static int iniciado;
static unsigned char *const disponibles=(void *)0x600000;
static unsigned char *const entregadas=(void *)(0x600000+BYTES);
static struct nxk_region_ram copia[32];
static int bit(const unsigned char *mapa, uint32_t n) { return !!(mapa[n/8] & (1u<<(n%8))); }
static void marcar(unsigned char *mapa, uint32_t n, int valor) {
    if (valor) mapa[n/8]|=(unsigned char)(1u<<(n%8));
    else mapa[n/8]&=(unsigned char)~(1u<<(n%8));
}
#endif
int nxk_paginas_iniciar(const struct nxk_region_ram *mapa, unsigned cantidad) {
#if defined(__i386__)
    /* No reinicializar mientras alguien conserve paginas del gestor anterior. */
    if (!mapa || !cantidad || cantidad>32 || entregas_activas) return -1;
    memcpy(copia,mapa,cantidad*sizeof(*mapa));
    for (unsigned i=0;i<cantidad;++i) {
        uint64_t a=copia[i].base, n=copia[i].longitud;
        if (n>UINT64_MAX-a) return -2;
        uint64_t b=a+n;
        if (n && copia[i].tipo!=1 && a<0x610000 && b>0x600000) return -3;
    }
    /* El BIOS puede partir una region RAM justo en la zona de metadatos.
       Exigimos que cada pagina de los 64 KiB este cubierta completamente
       por alguna region utilizable; regiones reservadas ya fueron rechazadas. */
    for (uint32_t m=0x600000;m<0x610000;m+=4096) {
        int cubierta=0;
        for (unsigned i=0;i<cantidad && !cubierta;++i) {
            uint64_t base=copia[i].base, longitud=copia[i].longitud;
            if (copia[i].tipo==1 && base<=m && base+longitud>=m+4096) cubierta=1;
        }
        if (!cubierta) return -4;
    }
    /* Solo ahora podemos escribir en la zona destinada a los mapas de bits. */
    memset(disponibles,0,BYTES); memset(entregadas,0,BYTES);
    for (unsigned paso=0;paso<2;++paso) {
        for (unsigned i=0;i<cantidad;++i) {
            int util=copia[i].tipo==1;
            if ((paso==0)!=util || !copia[i].longitud || copia[i].base>=LIMITE) continue;
            uint64_t a=copia[i].base, b=a+copia[i].longitud;
            if (b>LIMITE) b=LIMITE;
            /* RAM: solo paginas completas. Reservas: cualquier pagina tocada. */
            uint32_t primera=(uint32_t)((a+(util ? 4095 : 0))/4096);
            uint32_t fin=(uint32_t)((b+(util ? 0 : 4095))/4096);
            if (primera<PRIMERA) primera=PRIMERA;
            for (uint32_t p=primera;p<fin;++p) marcar(disponibles,p,util);
        }
    }
    total=0;
    for (uint32_t p=PRIMERA;p<CANTIDAD;++p) total+=bit(disponibles,p);
    libres=total; pista=PRIMERA;
    entregas_activas=0; iniciado=1;
    return 0;
#else
    (void)mapa; (void)cantidad; return -64;
#endif
}
int nxk_paginas_bios(void) {
#if defined(__i386__)
    volatile const uint16_t *cabecera=(const void *)0x5000;
    if (cabecera[1]!=1) return -1;
    return nxk_paginas_iniciar((const void *)0x5010,cabecera[0]);
#else
    return -64;
#endif
}
void *nxk_pagina_reservar(void) {
#if defined(__i386__)
    if (!iniciado || !libres) return 0;
    for (uint32_t p=pista;p<CANTIDAD;++p) {
        if (!bit(disponibles,p)) continue;
        marcar(disponibles,p,0); marcar(entregadas,p,1);
        --libres; ++entregas_activas; pista=p+1;
        void *pagina=(void *)(uintptr_t)(p*4096);
        /* No filtrar a un nuevo usuario los bytes del anterior. */
        memset(pagina,0,4096);
        return pagina;
    }
#endif
    return 0;
}
int nxk_pagina_liberar(void *direccion) {
#if defined(__i386__)
    if (!iniciado) return -3;
    uintptr_t a=(uintptr_t)direccion;
    if ((a&4095) || a<0x800000 || a>=LIMITE || !total) return -1;
    uint32_t p=(uint32_t)(a/4096);
    if (!bit(entregadas,p)) return -2;
    /* Borrar antes de marcarla libre para que el siguiente uso no vea restos. */
    memset((void *)a,0,4096);
    marcar(entregadas,p,0); marcar(disponibles,p,1); ++libres; --entregas_activas;
    if (p<pista) pista=p;
    return 0;
#else
    (void)direccion; return -64;
#endif
}
int nxk_paginas_reservar_region(uint32_t inicio, uint32_t longitud) {
#if defined(__i386__)
    /* Nunca acceder a metadatos fijos antes de validar su RAM mediante BIOS. */
    if (!iniciado) return -3;
    if (!longitud) return 0;
    uint64_t fin=(uint64_t)inicio+longitud;
    if (fin>0x40000000ull) return -1;
    uint32_t primero=inicio/4096, despues=(uint32_t)((fin+4095)/4096);
    if (primero<PRIMERA) primero=PRIMERA;
    /* Validacion previa: si alguna pagina esta en manos de un llamador,
       no cambiamos ninguna parte del rango. */
    for (uint32_t p=primero;p<despues;++p)
        if (bit(entregadas,p)) return -2;
    for (uint32_t p=primero;p<despues;++p) {
        if (bit(disponibles,p)) { marcar(disponibles,p,0); --libres; }
    }
    if (pista>=primero && pista<despues) {
        pista=despues;
        while (pista<CANTIDAD && !bit(disponibles,pista)) ++pista;
    }
    return 0;
#else
    (void)inicio; (void)longitud; return -64;
#endif
}
uint32_t nxk_paginas_libres(void) { return libres; }
uint32_t nxk_paginas_total(void) { return total; }
