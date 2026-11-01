/* Paginacion x86 inicial, puente C. Identidad 0..1 GiB, solo supervisor.
 * Tablas en RAM reservada bajo 8 MiB: no se entregan por el gestor fisico.
 * No hay espacios por proceso ni NX en este modo de paginacion. */
#include "paginas.h"
#include "paginacion.h"
#if defined(__i386__)
extern unsigned char __sololectura_fin[];
static int activa;
static uint32_t guardas;
static int ram_tablas(void) {
    volatile const uint16_t *h=(const void *)0x5000;
    const struct nxk_region_ram *m=(const void *)0x5010;
    if (h[1]!=1 || !h[0] || h[0]>32) return 0;
    for (uint32_t p=0x200000;p<0x301000;p+=4096) {
        int cubierta=0;
        for (unsigned i=0;i<h[0];++i) {
            uint64_t a=m[i].base,n=m[i].longitud;
            if (n>UINT64_MAX-a) return 0;
            uint64_t b=a+n;
            if (a<p+4096 && b>p && m[i].tipo!=1) return 0;
            if (m[i].tipo==1 && a<=p && b>=p+4096) cubierta=1;
        }
        if (!cubierta) return 0;
    }
    return 1;
}
int nxk_paginacion_iniciar(void) {
    if (!ram_tablas()) return -1;
    uint32_t *directorio=(void *)0x200000, *tablas=(void *)0x201000;
    memset(directorio,0,4096);
    for (uint32_t i=0;i<262144;++i) {
        uint32_t a=i*4096;
        uint32_t atributos=(a>=0x10000 && a<(uintptr_t)__sololectura_fin) ? 1 : 3;
        tablas[i]=a|atributos;
    }
    tablas[0]=0; /* Detectar accesos nulos; VGA y BIOS siguen mapeados. */
    for (unsigned i=0;i<256;++i) directorio[i]=(0x201000+i*4096)|3;
    uint32_t control;
    __asm__ volatile("mov %0,%%cr3" :: "r"(0x200000u) : "memory");
    __asm__ volatile("mov %%cr0,%0":"=r"(control));
    control|=0x80010000u; /* PG y WP: incluso ring0 respeta solo lectura. */
    __asm__ volatile("mov %0,%%cr0" :: "r"(control) : "memory");
    activa=1;
    return 0;
}
uint32_t nxk_paginacion_guardas(void) { return guardas; }
static int pagina_heap(uintptr_t a) {
    return activa && !(a&4095) && a>=0x100000 && a<0x180000 &&
        a+4096<=0x100000+nxk_heap_used();
}
int nxk_paginacion_guarda(void *pagina) {
    uintptr_t a=(uintptr_t)pagina;
    uint32_t cr0; __asm__ volatile("mov %%cr0,%0":"=r"(cr0));
    if (!pagina_heap(a) || !(cr0&0x80000000u)) return -1;
    volatile uint32_t *tabla=(void *)0x201000;
    if (tabla[a/4096]&1) ++guardas;
    tabla[a/4096]&=~1u;
    __asm__ volatile("invlpg (%0)" :: "r"(a) : "memory");
    return 0;
}
int nxk_paginacion_es_guarda(void *pagina) {
    uintptr_t a=(uintptr_t)pagina;
    if (!pagina_heap(a)) return 0;
    volatile uint32_t *tabla=(void *)0x201000;
    return (tabla[a/4096]&0xfffff001u)==a;
}

#else
uint32_t nxk_paginacion_guardas(void) { return 0; }
int nxk_paginacion_iniciar(void) { return -64; }
int nxk_paginacion_guarda(void *p) { (void)p; return -64; }
int nxk_paginacion_es_guarda(void *p) { (void)p; return 0; }
#endif
