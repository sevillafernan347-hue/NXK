/* Paginacion x86 inicial, puente C. Identidad 0..1 GiB, solo supervisor.
 * Tablas en RAM reservada bajo 8 MiB: no se entregan por el gestor fisico.
 * No hay espacios por proceso ni NX en este modo de paginacion. */
#include "paginas.h"
#include "paginacion.h"
#if defined(__i386__)
extern unsigned char __sololectura_fin[];
static int activa;
static uint32_t guardas;
/* Comprobar la union de RAM util, sin asignar ni modificar el mapa BIOS.
 * Basta un byte reservado o un hueco para rechazar la zona de tablas. */
static int mapa_cubre_tablas(const struct nxk_region_ram *m,unsigned n) {
    if (!m || !n || n>32) return 0;
    for (unsigned i=0;i<n;++i) {
        uint64_t a=m[i].base,longitud=m[i].longitud;
        if (longitud>UINT64_MAX-a) return 0;
        if (longitud && m[i].tipo!=1 && a<0x301000 && a+longitud>0x200000) return 0;
    }
    uint64_t cubierto=0x200000;
    /* Cada vuelta extiende el prefijo por al menos una region nueva.
     * Los datos pueden llegar desordenados o partidos dentro de una pagina. */
    for (unsigned vuelta=0;vuelta<n && cubierto<0x301000;++vuelta) {
        uint64_t anterior=cubierto;
        for (unsigned i=0;i<n;++i) {
            uint64_t a=m[i].base,fin=a+m[i].longitud;
            if (m[i].tipo==1 && a<=cubierto && fin>cubierto) cubierto=fin;
        }
        if (cubierto==anterior) return 0;
    }
    return cubierto>=0x301000;
}
static int ram_tablas(void) {
    volatile const uint16_t *h=(const void *)0x5000;
    if (h[1]!=1) return 0;
    return mapa_cubre_tablas((const void *)0x5010,h[0]);
}
int nxk_paginacion_iniciar(void) {
    /* Nunca reconstruir tablas activas: borraria guardas y permisos. */
    if (activa) return -2;
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

#ifdef NXK_TEST
static uint32_t huella_tablas(void) {
    volatile const uint32_t *p=(const void *)0x200000;
    uint32_t h=2166136261u;
    for (unsigned i=0;i<0x101000/4;++i)
        /* La CPU puede cambiar accessed/dirty mientras se lee el mapa. */
        h=(h^(p[i]&~0x60u))*16777619u;
    return h;
}
int nxk_paginacion_probar(void) {
    struct nxk_region_ram m[]={
        {0x280123,0x80edd,1}, {0x200000,0x123,1},
        {0x200123,0x80000,1}, {0x200800,0,2}
    };
    if (!mapa_cubre_tablas(m,4) || mapa_cubre_tablas(0,4) ||
        mapa_cubre_tablas(m,0) || mapa_cubre_tablas(m,33)) return 1;
    m[2].base++; m[2].longitud--;
    if (mapa_cubre_tablas(m,4)) return 2; /* Hueco de un byte. */
    m[2].base--; m[2].longitud++;
    m[3].longitud=1;
    if (mapa_cubre_tablas(m,4)) return 3; /* Reserva de un byte prevalece. */
    m[3]=(struct nxk_region_ram){0x301000,4096,2};
    if (!mapa_cubre_tablas(m,4)) return 4; /* Reserva fuera del limite. */
    m[3]=(struct nxk_region_ram){UINT64_MAX,1,1};
    if (mapa_cubre_tablas(m,4)) return 5;
    m[3]=(struct nxk_region_ram){0x200000,0x101000,1};
    if (!mapa_cubre_tablas(m,4)) return 6; /* Solapamiento util completo. */
    if (!activa) return 7;
    uint32_t antes=huella_tablas(), g=guardas, cr3, cr0;
    __asm__ volatile("mov %%cr3,%0":"=r"(cr3));
    __asm__ volatile("mov %%cr0,%0":"=r"(cr0));
    if (nxk_paginacion_iniciar()!=-2 || guardas!=g || huella_tablas()!=antes) return 8;
    uint32_t despues3,despues0;
    __asm__ volatile("mov %%cr3,%0":"=r"(despues3));
    __asm__ volatile("mov %%cr0,%0":"=r"(despues0));
    return cr3!=despues3 || cr0!=despues0 ? 9 : 0;
}
#endif

#else
uint32_t nxk_paginacion_guardas(void) { return 0; }
int nxk_paginacion_iniciar(void) { return -64; }
int nxk_paginacion_guarda(void *p) { (void)p; return -64; }
int nxk_paginacion_es_guarda(void *p) { (void)p; return 0; }
#endif
