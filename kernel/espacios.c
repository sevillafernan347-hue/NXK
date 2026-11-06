/* Cuatro directorios privados sin consumir heap. Cada espacio posee cinco
 * paginas fisicas contiguas: PD/PT0/PT1/codigo/pila. Nunca liberar un CR3 activo.
 * Solo las dos paginas virtuales 400000/401000 reciben permiso de usuario.
 * Sin NX (paging x86 no PAE), SMP, concurrencia ni recursos de proceso. */
#include "espacios.h"
#include "paginas.h"
#include "tareas.h"
#include "paginacion.h"
#if defined(__i386__)
#define KERNEL_CR3 0x200000u
struct espacio { uint32_t generacion,id,base; };
static struct espacio espacios[NXK_ESPACIOS_MAX];
static uint32_t cr3(void) { uint32_t n; __asm__ volatile("mov %%cr3,%0":"=r"(n)); return n; }
static int paginado(void) { uint32_t n; __asm__ volatile("mov %%cr0,%0":"=r"(n)); return (n&0x80000000u)!=0; }
static struct espacio *buscar(uint32_t id) {
    unsigned i=(id&255u)-1;
    return i<NXK_ESPACIOS_MAX && espacios[i].base && espacios[i].id==id ? &espacios[i] : 0;
}
static int contexto(void) {
    if (!paginado() || nxk_tarea_actual()) return 0;
    uint32_t actual=cr3(); if (actual==KERNEL_CR3) return 1;
    for (unsigned i=0;i<NXK_ESPACIOS_MAX;++i)
        if (espacios[i].base && espacios[i].base==actual) return 1;
    return 0;
}
/* Copiar desde el mapa kernel original: conserva guardas nuevas al activar.
 * Las paginas fisicas >=8MiB siguen accesibles por alias solo supervisor. */
static void preparar(struct espacio *e) {
    uint32_t *d=(void *)(uintptr_t)e->base, *baja=(void *)(uintptr_t)(e->base+4096),
        *alta=(void *)(uintptr_t)(e->base+8192);
    memcpy(d,(void *)KERNEL_CR3,4096);
    memcpy(baja,(void *)0x201000,4096); memcpy(alta,(void *)0x202000,4096);
    for (unsigned i=0;i<1024;++i) { d[i]&=~4u; baja[i]&=~4u; alta[i]&=~4u; }
    d[0]=(e->base+4096)|3u; d[1]=(e->base+8192)|7u;
    alta[0]=(e->base+12288)|5u; alta[1]=(e->base+16384)|7u;
}
int nxk_espacio_crear(const void *codigo,size_t n,uint32_t *id) {
    if (!codigo || !n || n>4096 || !id) return -1;
    if (!contexto() || cr3()!=KERNEL_CR3) return -3;
    unsigned i;
    for (i=0;i<NXK_ESPACIOS_MAX;++i) if (!espacios[i].base && espacios[i].generacion<0xffffffu) break;
    if (i==NXK_ESPACIOS_MAX) return -2;
    void *reserva=nxk_paginas_reservar_bloque(5,1); if (!reserva) return -4;
    struct espacio *e=&espacios[i]; e->base=(uint32_t)(uintptr_t)reserva;
    memcpy((void *)(uintptr_t)(e->base+12288),codigo,n); preparar(e);
    ++e->generacion; e->id=(e->generacion<<8)|(i+1); *id=e->id; return 0;
}
int nxk_espacio_consultar(uint32_t id,struct nxk_espacio_info *salida) {
    struct espacio *e=buscar(id); if (!e || !salida) return -1;
    *salida=(struct nxk_espacio_info){e->id,e->base,e->base+12288,e->base+16384}; return 0;
}
int nxk_espacio_activar(uint32_t id) {
    struct espacio *e=buscar(id); if (!e) return -1;
    if (!contexto()) return -3;
    /* Reconstruir solo con directorio kernel activo, nunca tablas en uso. */
    __asm__ volatile("mov %0,%%cr3"::"r"(KERNEL_CR3):"memory");
    preparar(e);
    __asm__ volatile("mov %0,%%cr3"::"r"(e->base):"memory"); return 0;
}
int nxk_espacio_restaurar(void) {
    if (!contexto()) return -3;
    __asm__ volatile("mov %0,%%cr3"::"r"(KERNEL_CR3):"memory"); return 0;
}
int nxk_espacio_destruir(uint32_t id) {
    struct espacio *e=buscar(id); if (!e) return -1;
    if (paginado() && cr3()==e->base) return -2;
    if (!contexto() || cr3()!=KERNEL_CR3) return -3;
    if (nxk_paginas_liberar_bloque((void *)(uintptr_t)e->base,5)) return -4;
    e->base=0; e->id=0; return 0;
}
/* Demostracion kernel: cuatro pilas en la misma VA, contenido independiente.
 * Toda salida de error restaura CR3 y devuelve reservas adquiridas. */
int nxk_espacios_demo(void) {
    uint32_t ids[NXK_ESPACIOS_MAX]={0}, libres=nxk_paginas_libres();
    size_t heap=nxk_heap_used(); int r=0;
    if (!contexto() || cr3()!=KERNEL_CR3) return -3;
    for (unsigned i=0;i<NXK_ESPACIOS_MAX;++i) {
        unsigned char codigo=(unsigned char)(0xa0+i);
        if (nxk_espacio_crear(&codigo,1,&ids[i])) { r=1; break; }
    }
    for (unsigned vuelta=0;!r && vuelta<3;++vuelta) for (unsigned i=0;i<NXK_ESPACIOS_MAX;++i) {
        if (nxk_espacio_activar(ids[i])) { r=2; break; }
        volatile unsigned char *codigo=(void *)NXK_ESPACIO_CODIGO;
        volatile uint32_t *pila=(void *)NXK_ESPACIO_PILA;
        if (*codigo!=0xa0+i || *pila!=(vuelta ? 100+i : 0)) { r=3; break; }
        *pila=100+i;
        if (nxk_espacio_destruir(ids[i])!=-2) { r=4; break; }
    }
    if (nxk_espacio_restaurar()) r=5;
    for (unsigned i=0;i<NXK_ESPACIOS_MAX;++i) if (ids[i] && nxk_espacio_destruir(ids[i])) r=6;
    return r || nxk_paginas_libres()!=libres || nxk_heap_used()!=heap ? -1 : 0;
}
#ifdef NXK_TEST
/* Inyecta una guarda posterior a crear el espacio y la restaura siempre.
 * No accede al contenido de esa pagina; solo compara PTEs. */
static int probar_guarda(uint32_t id) {
    volatile uint32_t *pte=(void *)(0x201000u+(0x17f000u/4096)*4);
    uint32_t original=*pte; *pte=original&~1u;
    __asm__ volatile("invlpg (%0)"::"r"(0x17f000u):"memory");
    int r=nxk_espacio_activar(id);
    struct espacio *e=buscar(id);
    if (!r && (((uint32_t *)(uintptr_t)(e->base+4096))[0x17f000u/4096]&1u)) r=1;
    uint32_t guardas=nxk_paginacion_guardas();
    if (!r && (nxk_paginacion_guarda((void *)0x17e000u)!=-1 || nxk_paginacion_guardas()!=guardas)) r=1;
    if (nxk_espacio_restaurar()) r=1;
    *pte=original; __asm__ volatile("invlpg (%0)"::"r"(0x17f000u):"memory");
    return r;
}
int nxk_espacios_probar(void) {
    uint32_t ids[NXK_ESPACIOS_MAX]={0}, salida=0xdeadbeef, libres=nxk_paginas_libres();
    size_t heap=nxk_heap_used(); unsigned char codigo=0xc3;
    if (nxk_espacio_crear(0,1,&salida)!=-1 || nxk_espacio_crear(&codigo,0,&salida)!=-1 ||
        nxk_espacio_crear(&codigo,4097,&salida)!=-1 || nxk_espacio_crear(&codigo,1,0)!=-1 || salida!=0xdeadbeef) return 1;
    for (unsigned i=0;i<NXK_ESPACIOS_MAX;++i) if (nxk_espacio_crear(&codigo,1,&ids[i])) return 2;
    if (nxk_paginas_libres()!=libres-20 || nxk_espacio_crear(&codigo,1,&salida)!=-2 || salida!=0xdeadbeef) return 3;
    for (unsigned i=0;i<NXK_ESPACIOS_MAX;++i) {
        struct nxk_espacio_info info;
        if (nxk_espacio_consultar(ids[i],&info)) return 4;
        uint32_t *d=(void *)(uintptr_t)info.directorio,*baja=(void *)(uintptr_t)(info.directorio+4096),
            *alta=(void *)(uintptr_t)(info.directorio+8192);
        if ((d[0]&7)!=3 || (d[1]&7)!=7 || baja[0] ||
            (alta[0]&~0x60u)!=(info.codigo_fisico|5u) || (alta[1]&~0x60u)!=(info.pila_fisica|7u)) return 5;
        for (unsigned j=2;j<1024;++j) if (d[j]&4u || alta[j]&4u) return 6;
        for (unsigned j=0;j<1024;++j) if (baja[j]&4u) return 7;
        for (unsigned j=1;j<4096;++j) if (((unsigned char *)(uintptr_t)info.codigo_fisico)[j]) return 8;
        for (unsigned j=0;j<4096;++j) if (((unsigned char *)(uintptr_t)info.pila_fisica)[j]) return 9;
    }
    if (probar_guarda(ids[0])) return 14;
    uint32_t viejo=ids[0];
    for (unsigned i=0;i<NXK_ESPACIOS_MAX;++i) if (nxk_espacio_destruir(ids[i])) return 10;
    struct nxk_espacio_info info; memset(&info,0xa5,sizeof(info));
    if (nxk_espacio_consultar(viejo,&info)!=-1 || nxk_espacio_activar(viejo)!=-1 || nxk_espacio_destruir(viejo)!=-1) return 11;
    for (unsigned i=0;i<sizeof(info);++i) if (((unsigned char *)&info)[i]!=0xa5) return 12;
    if (nxk_espacio_crear(&codigo,1,&ids[0]) || ids[0]==viejo || nxk_espacio_destruir(ids[0]) ||
        nxk_espacios_demo() || nxk_espacios_demo() || nxk_paginas_libres()!=libres || nxk_heap_used()!=heap) return 13;
    return 0;
}
#endif
#else
int nxk_espacio_crear(const void *c,size_t n,uint32_t *id) { (void)c;(void)n;(void)id;return -64; }
int nxk_espacio_consultar(uint32_t id,struct nxk_espacio_info *s) { (void)id;(void)s;return -64; }
int nxk_espacio_activar(uint32_t id) { (void)id;return -64; }
int nxk_espacio_restaurar(void) { return -64; }
int nxk_espacio_destruir(uint32_t id) { (void)id;return -64; }
int nxk_espacios_demo(void) { return -64; }
#endif
