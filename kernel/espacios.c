/* Cuatro directorios privados sin consumir heap. Cinco paginas contiguas por
 * espacio: PD/PT0/PT1/codigo/pila, mas hasta cuatro paginas de datos opcionales.
 * Nunca liberar un CR3 activo. Solo codigo, pila y datos propios son usuario.
 * Sin NX (paging x86 no PAE), SMP ni concurrencia en el asignador fisico. */
#include "espacios.h"
#include "paginas.h"
#include "tareas.h"
#include "paginacion.h"
#include "procesos.h"
#if defined(__i386__)
#define KERNEL_CR3 0x200000u
struct espacio { uint32_t generacion,id,base,datos[NXK_ESPACIO_DATOS_MAX]; };
static struct espacio espacios[NXK_ESPACIOS_MAX];
/* Tres bits por slot: cuotas0..4. No altera el ID ni su generacion. */
_Static_assert(NXK_ESPACIOS_MAX*3<=16 && NXK_ESPACIO_DATOS_MAX<8,"Cuotas fuera de rango");
static uint16_t cuotas;
static uint32_t limite(const struct espacio *e) { return (cuotas>>(3*(e-espacios)))&7u; }
static void poner_limite(const struct espacio *e,uint32_t n) {
    unsigned desplazamiento=3*(unsigned)(e-espacios);
    cuotas=(uint16_t)((cuotas&~(7u<<desplazamiento))|(n<<desplazamiento));
}
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
    for (unsigned i=0;i<NXK_ESPACIO_DATOS_MAX;++i)
        if (e->datos[i]) alta[2+i]=e->datos[i]|7u;
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
    poner_limite(e,NXK_ESPACIO_DATOS_MAX);
    ++e->generacion; e->id=(e->generacion<<8)|(i+1); *id=e->id; return 0;
}
int nxk_espacio_consultar(uint32_t id,struct nxk_espacio_info *salida) {
    struct espacio *e=buscar(id); if (!e || !salida) return -1;
    uint32_t cantidad=0;
    for (unsigned i=0;i<NXK_ESPACIO_DATOS_MAX;++i) cantidad+=!!e->datos[i];
    uint32_t cuota=limite(e);
    if (cuota>NXK_ESPACIO_DATOS_MAX || cantidad>cuota) return -1;
    *salida=(struct nxk_espacio_info){e->id,e->base,e->base+12288,e->base+16384,cantidad,cuota}; return 0;
}
int nxk_espacio_enumerar(unsigned slot,struct nxk_espacio_info *salida) {
    if (slot>=NXK_ESPACIOS_MAX || !salida) return -1;
    if (!espacios[slot].base) return -2;
    return nxk_espacio_consultar(espacios[slot].id,salida);
}
/* Cambiar una PTE del espacio propio tambien funciona durante INT81: el
 * kernel conserva acceso supervisor al alias fisico. Otro CR3 privado no
 * puede reservar ni liberar paginas de este ID. No toca el directorio kernel. */
static int contexto_datos(const struct espacio *e) {
    return paginado() && !nxk_tarea_actual() && (cr3()==KERNEL_CR3 || cr3()==e->base);
}
static void actualizar_dato(struct espacio *e,unsigned i) {
    uint32_t virtual=NXK_ESPACIO_DATOS+i*4096;
    uint32_t *tabla=(void *)(uintptr_t)(e->base+8192);
    /* Al liberar se restaura el mapa supervisor original de esta direccion. */
    tabla[2+i]=e->datos[i] ? e->datos[i]|7u : ((uint32_t *)0x202000)[2+i]&~4u;
    if (cr3()==e->base) __asm__ volatile("invlpg (%0)"::"r"(virtual):"memory");
}
__attribute__((minsize)) int nxk_espacio_cuota(uint32_t id,uint32_t *salida) {
    struct espacio *e=buscar(id); if (!e || !salida || limite(e)>NXK_ESPACIO_DATOS_MAX) return -1;
    *salida=limite(e); return 0;
}
__attribute__((minsize)) int nxk_espacio_limitar_datos(uint32_t id,uint32_t cuota) {
    struct espacio *e=buscar(id); if (!e || cuota>NXK_ESPACIO_DATOS_MAX) return -1;
    if (!contexto_datos(e)) return -3;
    uint32_t anterior=limite(e),usadas=0;
    if (anterior>NXK_ESPACIO_DATOS_MAX) return -1;
    if (cuota>anterior) return -5;
    for (unsigned i=0;i<NXK_ESPACIO_DATOS_MAX;++i) usadas+=!!e->datos[i];
    if (cuota<usadas) return -2;
    poner_limite(e,cuota); return 0;
}
__attribute__((minsize)) int nxk_espacio_reservar_paginas(uint32_t id,uint32_t cantidad,uint32_t *direccion) {
    struct espacio *e=buscar(id);
    if (!e || !direccion || !cantidad || cantidad>NXK_ESPACIO_DATOS_MAX) return -1;
    if (!contexto_datos(e)) return -3;
    uint32_t usadas=0,cuota=limite(e);
    if (cuota>NXK_ESPACIO_DATOS_MAX) return -1;
    for (unsigned j=0;j<NXK_ESPACIO_DATOS_MAX;++j) usadas+=!!e->datos[j];
    if (usadas>cuota || cantidad>cuota-usadas) return -2;
    unsigned i;
    for (i=0;i<=NXK_ESPACIO_DATOS_MAX-cantidad;++i) {
        unsigned j=0; while (j<cantidad && !e->datos[i+j]) ++j;
        if (j==cantidad) break;
    }
    if (i>NXK_ESPACIO_DATOS_MAX-cantidad) return -2;
    uint32_t temporales[NXK_ESPACIO_DATOS_MAX];
    unsigned adquiridas=0;
    while (adquiridas<cantidad) {
        void *pagina=nxk_pagina_reservar();
        if (!pagina) {
            /* Ninguna PTE publicada todavia; restaurar el asignador. */
            while (adquiridas) nxk_pagina_liberar((void *)(uintptr_t)temporales[--adquiridas]);
            return -4;
        }
        temporales[adquiridas++]=(uint32_t)(uintptr_t)pagina;
    }
    for (unsigned j=0;j<cantidad;++j) {
        e->datos[i+j]=temporales[j]; actualizar_dato(e,i+j);
    }
    *direccion=NXK_ESPACIO_DATOS+i*4096; return 0;
}
int nxk_espacio_reservar_pagina(uint32_t id,uint32_t *direccion) {
    return nxk_espacio_reservar_paginas(id,1,direccion);
}
__attribute__((minsize)) int nxk_espacio_liberar_paginas(uint32_t id,uint32_t virtual,uint32_t cantidad) {
    struct espacio *e=buscar(id);
    if (!e || !cantidad || cantidad>NXK_ESPACIO_DATOS_MAX || virtual<NXK_ESPACIO_DATOS || (virtual&4095u) ||
        virtual>=NXK_ESPACIO_DATOS+NXK_ESPACIO_DATOS_MAX*4096) return -1;
    unsigned i=(virtual-NXK_ESPACIO_DATOS)/4096;
    if (cantidad>NXK_ESPACIO_DATOS_MAX-i) return -1;
    if (!contexto_datos(e)) return -3;
    for (unsigned j=0;j<cantidad;++j) {
        if (!e->datos[i+j]) return -1;
        if (nxk_proceso_pagina_retenida(id,virtual+j*4096)) return -5;
    }
    for (unsigned j=0;j<cantidad;++j) {
        if (nxk_pagina_liberar((void *)(uintptr_t)e->datos[i+j])) return -4;
        e->datos[i+j]=0; actualizar_dato(e,i+j);
    }
    return 0;
}
int nxk_espacio_liberar_pagina(uint32_t id,uint32_t virtual) {
    return nxk_espacio_liberar_paginas(id,virtual,1);
}
/* Restar despues de validar el inicio evita overflow de direccion+longitud.
 * Devuelve el alias fisico de la pagina propia; cero identifica un hueco. */
static uint32_t respaldo(const struct espacio *e,uint32_t virtual) {
    unsigned pagina=(virtual-NXK_ESPACIO_CODIGO)/4096;
    return pagina<2 ? e->base+12288+pagina*4096 : e->datos[pagina-2];
}
int nxk_espacio_validar_rango(uint32_t id,uint32_t virtual,size_t n,int escritura) {
    struct espacio *e=buscar(id); if (!e || (escritura!=0 && escritura!=1)) return -1;
    if (!contexto_datos(e)) return -3;
    uint32_t minimo=escritura ? NXK_ESPACIO_PILA : NXK_ESPACIO_CODIGO;
    uint32_t fin=NXK_ESPACIO_DATOS+NXK_ESPACIO_DATOS_MAX*4096;
    if (virtual<minimo || virtual>fin || n>fin-virtual) return -2;
    for (uint32_t p=virtual&~4095u;n && p<virtual+n;p+=4096)
        if (!respaldo(e,p)) return -2;
    return 0;
}
static int copiar(uint32_t id,uint32_t virtual,void *buffer,size_t n,int escritura) {
    struct espacio *e=buscar(id); if (!e || (!buffer && n)) return -1;
    if (cr3()!=KERNEL_CR3) return -3;
    int r=nxk_espacio_validar_rango(id,virtual,n,escritura); if (r) return r;
    while (n) {
        size_t tramo=4096-(virtual&4095u); if (tramo>n) tramo=n;
        void *fisico=(void *)(uintptr_t)(respaldo(e,virtual)+(virtual&4095u));
        if (escritura) memmove(fisico,buffer,tramo); else memmove(buffer,fisico,tramo);
        virtual+=(uint32_t)tramo; buffer=(unsigned char *)buffer+tramo; n-=tramo;
    }
    return 0;
}
int nxk_espacio_leer(uint32_t id,uint32_t virtual,void *destino,size_t n) { return copiar(id,virtual,destino,n,0); }
int nxk_espacio_escribir(uint32_t id,uint32_t virtual,const void *origen,size_t n) { return copiar(id,virtual,(void *)origen,n,1); }
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
    /* Un observador conserva el resultado de este ID hasta consumirlo o
     * cancelar su espera, tambien si entra por la API antigua de espacios. */
    if (nxk_proceso_retenido(id)) return -5;
    for (unsigned i=0;i<NXK_ESPACIO_DATOS_MAX;++i) if (e->datos[i]) {
        if (nxk_pagina_liberar((void *)(uintptr_t)e->datos[i])) return -4;
        e->datos[i]=0;
    }
    if (nxk_paginas_liberar_bloque((void *)(uintptr_t)e->base,5)) return -4;
    poner_limite(e,0); e->base=0; e->id=0; return 0;
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
/* Agotar RAM solo en configuraciones pequenas: deja una pagina fisica libre
 * y pide dos. El fallo debe devolver la temporal sin publicar ninguna PTE. */
__attribute__((minsize)) static int probar_lote_sin_ram(uint32_t id) {
    uint32_t libres=nxk_paginas_libres(),cabeza=0,salida=0xdeadbeef;
    if (libres>8192) return 0;
    void *pagina;
    while ((pagina=nxk_pagina_reservar())) {
        *(uint32_t *)pagina=cabeza; cabeza=(uint32_t)(uintptr_t)pagina;
    }
    if (!cabeza) return 1;
    uint32_t unica=cabeza; cabeza=*(uint32_t *)(uintptr_t)cabeza;
    int fallo=nxk_pagina_liberar((void *)(uintptr_t)unica);
    if (nxk_espacio_reservar_paginas(id,2,&salida)!=-4 || salida!=0xdeadbeef ||
        nxk_paginas_libres()!=1 || nxk_espacio_validar_rango(id,NXK_ESPACIO_DATOS,1,1)!=-2) fallo=1;
    while (cabeza) {
        uint32_t siguiente=*(uint32_t *)(uintptr_t)cabeza;
        if (nxk_pagina_liberar((void *)(uintptr_t)cabeza)) fallo=1;
        cabeza=siguiente;
    }
    return fallo || nxk_paginas_libres()!=libres;
}
/* Inyecta una guarda posterior a crear el espacio y la restaura siempre.
 * No accede al contenido de esa pagina; solo compara PTEs. */
__attribute__((minsize)) static int probar_guarda(uint32_t id) {
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
__attribute__((minsize)) static int probar_copias(uint32_t a,uint32_t b) {
    unsigned char salida[4]={0xa5,0xa5,0xa5,0xa5}, valor=0x7b;
    uint32_t base=NXK_ESPACIO_CODIGO,pila=NXK_ESPACIO_PILA,fin=pila+4096;
    if (nxk_espacio_leer(a,base,salida,1) || salida[0]!=0xc3 ||
        nxk_espacio_escribir(a,pila,&valor,1) || nxk_espacio_leer(a,pila-1,salida,2) ||
        salida[0] || salida[1]!=valor || nxk_espacio_leer(b,pila,salida,1) || salida[0]) return 1;
    memset(salida,0xa5,sizeof(salida));
    if (nxk_espacio_leer(a,fin-1,salida,2)!=-2 || nxk_espacio_leer(a,UINT32_MAX,salida,1)!=-2 ||
        nxk_espacio_leer(a,base,salida,(size_t)-1)!=-2 || nxk_espacio_leer(a,base-1,salida,1)!=-2 ||
        nxk_espacio_escribir(a,base,&valor,1)!=-2 || nxk_espacio_escribir(a,pila-1,&valor,1)!=-2 ||
        nxk_espacio_escribir(a,fin-1,&valor,2)!=-2 || nxk_espacio_leer(a,pila,0,1)!=-1 ||
        nxk_espacio_escribir(a,pila,0,1)!=-1) return 2;
    for (unsigned i=0;i<sizeof(salida);++i) if (salida[i]!=0xa5) return 3;
    if (nxk_espacio_leer(a,fin,0,0) || nxk_espacio_escribir(a,fin,0,0) ||
        nxk_espacio_leer(a,pila,salida,1) || salida[0]!=valor ||
        nxk_espacio_leer(a,base,salida,1) || salida[0]!=0xc3 || nxk_espacio_activar(a)) return 4;
    int r=nxk_espacio_leer(b,pila,salida,1);
    int w=nxk_espacio_escribir(b,pila,&valor,1);
    if (nxk_espacio_restaurar() || r!=-3 || w!=-3) return 5;
    return 0;
}
/* Prueba pequena de cuota, copia atomica y CR3 propio. La cobertura extensa
 * usa NXE externos para conservar el limite de RAM de la imagen de pruebas. */
__attribute__((minsize)) static int probar_datos(uint32_t a,uint32_t b) {
    uint32_t paginas[4]={0},otra=0,salida=0xdeadbeef,libres=nxk_paginas_libres();
    uint32_t origen[2]={0x11223344,0x55667788},destino[2];
    int fallo=1;
    if (nxk_espacio_reservar_pagina(a,&paginas[0]) ||
        nxk_espacio_reservar_pagina(b,&otra)) goto limpiar;
    /* La reserva intercalada de b separa fisicamente las dos primeras de a. */
    for (unsigned i=1;i<4;++i)
        if (nxk_espacio_reservar_pagina(a,&paginas[i])) goto limpiar;
    if (nxk_espacio_reservar_pagina(a,&salida)!=-2 || salida!=0xdeadbeef ||
        nxk_paginas_libres()!=libres-5 ||
        nxk_espacio_escribir(a,paginas[0]+4092,origen,8) ||
        nxk_espacio_leer(a,paginas[0]+4092,destino,8) ||
        destino[0]!=origen[0] || destino[1]!=origen[1] ||
        nxk_espacio_liberar_pagina(a,paginas[1])) goto limpiar;
    destino[0]=destino[1]=0xa5a5a5a5;
    if (nxk_espacio_leer(a,paginas[0]+4092,destino,8)!=-2 ||
        nxk_espacio_escribir(a,paginas[0]+4092,destino,8)!=-2 ||
        destino[0]!=0xa5a5a5a5 || destino[1]!=0xa5a5a5a5 ||
        nxk_espacio_leer(a,paginas[0]+4092,destino,4) || destino[0]!=origen[0] ||
        nxk_espacio_activar(a) || nxk_espacio_liberar_pagina(b,otra)!=-3 ||
        nxk_espacio_reservar_pagina(b,&salida)!=-3 ||
        nxk_espacio_reservar_pagina(a,&salida) || salida!=paginas[1]) goto limpiar;
    for (unsigned i=0;i<4096;++i)
        if (((volatile unsigned char *)(uintptr_t)salida)[i]) goto limpiar;
    fallo=0;
limpiar:
    if (nxk_espacio_restaurar()) return 1;
    for (unsigned i=0;i<4;++i) if (paginas[i]) {
        int r=nxk_espacio_liberar_pagina(a,paginas[i]); if (r && r!=-1) fallo=1;
    }
    if (otra && nxk_espacio_liberar_pagina(b,otra)) fallo=1;
    if (nxk_paginas_libres()!=libres) fallo=1;
    return fallo;
}
__attribute__((minsize)) int nxk_espacios_probar(void) {
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
    if (probar_copias(ids[0],ids[1])) return 15;
    if (probar_datos(ids[0],ids[1])) return 16;
    if (probar_lote_sin_ram(ids[0])) return 18;
    uint32_t viejo=ids[0];
    /* Destruir debe recuperar reservas que el propietario no libero. */
    if (nxk_espacio_reservar_pagina(ids[0],&salida) ||
        nxk_espacio_reservar_pagina(ids[1],&salida)) return 17;
    for (unsigned i=0;i<NXK_ESPACIOS_MAX;++i) if (nxk_espacio_destruir(ids[i])) return 10;
    struct nxk_espacio_info info; memset(&info,0xa5,sizeof(info));
    salida=0xdeadbeef;
    if (nxk_espacio_reservar_pagina(viejo,&salida)!=-1 || salida!=0xdeadbeef ||
        nxk_espacio_liberar_pagina(viejo,NXK_ESPACIO_DATOS)!=-1 ||
        nxk_espacio_enumerar(0,&info)!=-2 || nxk_espacio_enumerar(NXK_ESPACIOS_MAX,&info)!=-1 ||
        nxk_espacio_enumerar(0,0)!=-1 || nxk_espacio_consultar(viejo,&info)!=-1 || nxk_espacio_activar(viejo)!=-1 || nxk_espacio_destruir(viejo)!=-1) return 11;
    for (unsigned i=0;i<sizeof(info);++i) if (((unsigned char *)&info)[i]!=0xa5) return 12;
    if (nxk_espacio_crear(&codigo,1,&ids[0]) || ids[0]==viejo || nxk_espacio_destruir(ids[0]) ||
        nxk_espacios_demo() || nxk_espacios_demo() || nxk_paginas_libres()!=libres || nxk_heap_used()!=heap) return 13;
    return 0;
}
#endif
#else
int nxk_espacio_reservar_paginas(uint32_t id,uint32_t n,uint32_t *p) { (void)id;(void)n;(void)p;return -64; }
int nxk_espacio_liberar_paginas(uint32_t id,uint32_t p,uint32_t n) { (void)id;(void)p;(void)n;return -64; }
int nxk_espacio_limitar_datos(uint32_t id,uint32_t n) { (void)id;(void)n;return -64; }
int nxk_espacio_cuota(uint32_t id,uint32_t *n) { (void)id;(void)n;return -64; }
int nxk_espacio_crear(const void *c,size_t n,uint32_t *id) { (void)c;(void)n;(void)id;return -64; }
int nxk_espacio_consultar(uint32_t id,struct nxk_espacio_info *s) { (void)id;(void)s;return -64; }
int nxk_espacio_enumerar(unsigned i,struct nxk_espacio_info *s) { (void)i;(void)s;return -64; }
int nxk_espacio_reservar_pagina(uint32_t id,uint32_t *v) { (void)id;(void)v;return -64; }
int nxk_espacio_liberar_pagina(uint32_t id,uint32_t v) { (void)id;(void)v;return -64; }
int nxk_espacio_validar_rango(uint32_t id,uint32_t v,size_t n,int e) { (void)id;(void)v;(void)n;(void)e;return -64; }
int nxk_espacio_leer(uint32_t id,uint32_t v,void *p,size_t n) { (void)id;(void)v;(void)p;(void)n;return -64; }
int nxk_espacio_escribir(uint32_t id,uint32_t v,const void *p,size_t n) { (void)id;(void)v;(void)p;(void)n;return -64; }
int nxk_espacio_activar(uint32_t id) { (void)id;return -64; }
int nxk_espacio_restaurar(void) { return -64; }
int nxk_espacio_destruir(uint32_t id) { (void)id;return -64; }
int nxk_espacios_demo(void) { return -64; }
#endif
