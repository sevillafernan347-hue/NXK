/* GUIA DE LECTURA NXK - kernel/pruebas_cargadores.c
 * Pruebas negativas: construye archivos incorrectos para comprobar que los cargadores los rechacen.
 * Contexto y orden de estudio: ver LEER-CODIGO.md en la carpeta principal.
 */
#include "nxe.h"
#include "pe.h"
extern const unsigned char nxe_demo_inicio[], nxe_demo_fin[];
#if defined(__i386__)
extern const unsigned char pe_demo_inicio[], pe_demo_fin[];
#endif
static void copiar(unsigned char *a, const unsigned char *b, size_t n) { while (n--) *a++=*b++; }
int nxk_probar_formatos(void) {
    unsigned char copia[4096];
    size_t n=(uintptr_t)nxe_demo_fin-(uintptr_t)nxe_demo_inicio;
    if (n>sizeof(copia)) return 1;
    for (size_t i=0; i<64; ++i) if (nxk_nxe_cargar(nxe_demo_inicio,i)!=-1) return 2;
    copiar(copia,nxe_demo_inicio,n); copia[0]='Z';
    if (nxk_nxe_cargar(copia,n)!=-1) return 3;
    copiar(copia,nxe_demo_inicio,n); copia[6]=(sizeof(void*)==4 ? 2 : 1);
    if (nxk_nxe_cargar(copia,n)!=-2) return 4;
    copiar(copia,nxe_demo_inicio,n); copia[n-1]^=1;
    if (nxk_nxe_cargar(copia,n)!=-6) return 5;
#if defined(__i386__)
    n=(uintptr_t)pe_demo_fin-(uintptr_t)pe_demo_inicio;
    if (n>sizeof(copia)) return 6;
    if (nxk_pe_cargar(pe_demo_inicio,63)!=-1) return 7;
    copiar(copia,pe_demo_inicio,n); copia[0]='Z';
    if (nxk_pe_cargar(copia,n)!=-1) return 8;
    copiar(copia,pe_demo_inicio,n);
    unsigned pe=copia[60] | (unsigned)copia[61]<<8 | (unsigned)copia[62]<<16 | (unsigned)copia[63]<<24;
    copia[pe+4]=0x64; copia[pe+5]=0x86;
    if (nxk_pe_cargar(copia,n)!=-2) return 9;
    /* PE32+ y subsistema nativo del kernel Windows no son apps Win32. */
    copiar(copia,pe_demo_inicio,n); copia[pe+24]=0x0b; copia[pe+25]=2;
    if (nxk_pe_cargar(copia,n)!=-3) return 12;
    copiar(copia,pe_demo_inicio,n); copia[pe+24+68]=1; copia[pe+24+69]=0;
    if (nxk_pe_cargar(copia,n)!=-5) return 13;
    /* Entrada en memoria virtual sin bytes de codigo respaldados: rechazar. */
    copiar(copia,pe_demo_inicio,n);
    unsigned opt=pe+24, tabla=opt+(copia[pe+20] | (unsigned)copia[pe+21]<<8);
    unsigned cuenta=copia[pe+6] | (unsigned)copia[pe+7]<<8;
    for (unsigned k=0;k<cuenta;++k) {
        unsigned q=tabla+k*40;
        if (!(copia[q+39]&0x20)) continue;
        for (unsigned b=0;b<4;++b) {
            copia[opt+16+b]=copia[q+12+b];
            copia[q+16+b]=0;
        }
        break;
    }
    if (nxk_pe_cargar(copia,n)!=-9) return 14;
    copiar(copia,pe_demo_inicio,n);
    unsigned i;
    for (i=0; i+12<n; ++i) {
        static const char nombre[]="GetStdHandle";
        unsigned j=0; while (j<12 && copia[i+j]==(unsigned char)nombre[j]) ++j;
        if (j==12) break;
    }
    if (i+12>=n) return 10;
    copia[i]='Z';
    if (nxk_pe_cargar(copia,n)!=-13) return 11;
#endif
    return 0;
}
