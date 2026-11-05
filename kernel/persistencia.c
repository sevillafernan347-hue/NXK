/* Persistencia original NXPD1: snapshot doble con cabecera de commit al final.
 * Puente C. FNV detecta corrupcion accidental, no es autenticacion.
 * No se escribe el disco de arranque ni se formatea desde el kernel. */
#include "persistencia.h"
#include "archivos.h"
#include "ata.h"
#include "tareas.h"
#if defined(__i386__)
static unsigned char datos[2][4096];
static struct nxk_archivo_imagen imagen[8];
static uint32_t generaciones[2];
static int validos[2];
static uint32_t u32(const unsigned char *p) { return p[0]|(uint32_t)p[1]<<8|(uint32_t)p[2]<<16|(uint32_t)p[3]<<24; }
static void poner(unsigned char *p,uint32_t n) { for (unsigned i=0;i<4;++i) p[i]=(unsigned char)(n>>(i*8)); }
static uint32_t huella(const unsigned char *p,unsigned n) { uint32_t h=2166136261u; while(n--) h=(h^*p++)*16777619u; return h; }
static int cero(const unsigned char *p,unsigned n) { while(n--) if (*p++) return 0; return 1; }
static int nombre(const unsigned char *p) {
    for (unsigned j=0;j<24;++j) {
        unsigned c=p[j];
        if (!c) return j && cero(p+j,24-j);
        if (!((c>='a'&&c<='z')||(c>='0'&&c<='9')||c=='.'||c=='_'||c=='-')) return 0;
    }
    return 0;
}
/* Retorna cantidad o -1. Exige formato canonico antes de importar RAM. */
static int validar_datos(unsigned banco) {
    unsigned n=0;
    for (unsigned i=0;i<8;++i) {
        const unsigned char *p=datos[banco]+512*i;
        if (!p[0]) { if (!cero(p,512)) return -1; continue; }
        uint32_t longitud=u32(p+24);
        if (!nombre(p) || longitud>256 || !cero(p+28,4) || !cero(p+32+longitud,480-longitud)) return -1;
        for (unsigned j=0;j<i;++j)
            if (nxk_equal((const char *)p,(const char *)datos[banco]+512*j)) return -1;
        ++n;
    }
    return (int)n;
}
/* Lee siempre ambos bancos; un error de E/S no equivale a banco invalido.
 * -1 disco, -2 formato, -3 sin copia valida/ambiguo, -4 E/S. */
static int inspeccionar(int *actual) {
    struct nxk_ata_info info; unsigned char h[512];
    if (nxk_ata_identificar(1,&info)) return -1;
    if (info.sectores!=19) return -2;
    if (nxk_ata_leer_sector(1,0,h)) return -4;
    if (u32(h)!=0x4450584eu || u32(h+4)!=0x31303030u || u32(h+8)!=1 ||
        u32(h+12)!=19 || u32(h+16)!=8 || u32(h+20)!=512 ||
        !cero(h+24,484) || u32(h+508)!=huella(h,508)) return -2;
    int vacios=0;
    for (unsigned b=0;b<2;++b) {
        validos[b]=0; generaciones[b]=0;
        if (nxk_ata_leer_sector(1,1+9*b,h)) return -4;
        for (unsigned i=0;i<8;++i)
            if (nxk_ata_leer_sector(1,2+9*b+i,datos[b]+512*i)) return -4;
        if (cero(h,512) && cero(datos[b],4096)) { ++vacios; continue; }
        int n=validar_datos(b);
        if (u32(h)!=0x5350584eu || u32(h+4)!=1 || !u32(h+8) || n<0 || u32(h+12)!=(uint32_t)n ||
            u32(h+16)!=huella(datos[b],4096) || !cero(h+20,488) || u32(h+508)!=huella(h,508)) continue;
        validos[b]=1; generaciones[b]=u32(h+8);
    }
    *actual=-1;
    if (validos[0] && validos[1] && generaciones[0]==generaciones[1]) return -3;
    if (validos[0]) *actual=0;
    if (validos[1] && (*actual<0 || generaciones[1]>generaciones[0])) *actual=1;
    return *actual<0 && vacios!=2 ? -3 : 0;
}
int nxk_persistencia_guardar(void) {
    if (nxk_tarea_actual()) return -7;
    int actual,r=inspeccionar(&actual); if (r) return r;
    uint32_t gen=actual<0 ? 0 : generaciones[actual];
    if (gen==UINT32_MAX) return -5; /* No volver a una generacion vieja. */
    if (nxk_archivos_exportar(imagen)) return -6;
    unsigned banco=actual<0 ? 0 : 1u-(unsigned)actual, n=0;
    memset(datos[banco],0,4096);
    for (unsigned i=0;i<8;++i) {
        unsigned char *p=datos[banco]+i*512;
        if (!imagen[i].nombre[0]) continue;
        memcpy(p,imagen[i].nombre,24); poner(p+24,imagen[i].longitud);
        memcpy(p+32,imagen[i].datos,256); ++n;
    }
    /* Escribir solo el banco inactivo. La cabecera anterior sigue intacta. */
    for (unsigned i=0;i<8;++i)
        if (nxk_ata_escribir_sector(1,2+9*banco+i,datos[banco]+i*512)) return -4;
    if (nxk_ata_sincronizar(1)) return -4;
    unsigned char h[512]; memset(h,0,512);
    poner(h,0x5350584e); poner(h+4,1); poner(h+8,gen+1); poner(h+12,n);
    poner(h+16,huella(datos[banco],4096)); poner(h+508,huella(h,508));
    if (nxk_ata_escribir_sector(1,1+9*banco,h) || nxk_ata_sincronizar(1)) return -4;
    /* Error despues del commit puede ser resultado incierto; releer decide. */
    int comprobado;
    r=inspeccionar(&comprobado);
    return r || comprobado!=(int)banco || generaciones[banco]!=gen+1 ? -4 : 0;
}
int nxk_persistencia_cargar(void) {
    if (nxk_tarea_actual()) return -7;
    int actual,r=inspeccionar(&actual); if (r) return r;
    if (actual<0) return 1;
    memset(imagen,0,sizeof(imagen));
    for (unsigned i=0;i<8;++i) {
        const unsigned char *p=datos[actual]+i*512;
        memcpy(imagen[i].nombre,p,24); imagen[i].longitud=u32(p+24);
        memcpy(imagen[i].datos,p+32,256);
    }
    return nxk_archivos_importar(imagen) ? -6 : 0;
}
#else
int nxk_persistencia_guardar(void) { return -64; }
int nxk_persistencia_cargar(void) { return -64; }
#endif
