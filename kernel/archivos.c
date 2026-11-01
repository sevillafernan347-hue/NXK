/* Sistema de archivos RAM propio. Puente C; sin disco ni concurrencia.
 * Validar antes de modificar conserva archivos ante operaciones rechazadas. */
#include "archivos.h"
#include "volumen.h"
struct archivo { char nombre[24]; unsigned char datos[256]; size_t n; };
static struct archivo *tabla;
static int valido(const char *s) {
    if (!s || !*s) return 0;
    for (unsigned i=0;i<24;++i) {
        unsigned char c=s[i];
        if (!c) return 1;
        if (!((c>='a' && c<='z') || (c>='0' && c<='9') || c=='_' || c=='-' || c=='.')) return 0;
    }
    return 0;
}
static int buscar(const char *s) {
    if (tabla) for (unsigned i=0;i<8;++i)
        if (tabla[i].nombre[0] && nxk_equal(s,tabla[i].nombre)) return (int)i;
    return -1;
}
int nxk_archivo_escribir(const char *s,const void *datos,size_t n) {
    if (!valido(s) || n>256 || (!datos && n)) return -1;
    if (!tabla) { tabla=nxk_alloc(8*sizeof(*tabla)); if (!tabla) return -2; }
    int i=buscar(s);
    if (i<0) for (unsigned j=0;j<8;++j) if (!tabla[j].nombre[0]) { i=(int)j; break; }
    if (i<0) return -3;
    /* Copiar primero permite reemplazar por datos procedentes del mismo archivo. */
    memmove(tabla[i].datos,datos,n);
    memset(tabla[i].datos+n,0,256-n);
    tabla[i].n=n;
    unsigned j=0; do { tabla[i].nombre[j]=s[j]; } while (s[j++]);
    return 0;
}
int nxk_archivo_leer(const char *s,void *datos,size_t capacidad) {
    if (!valido(s)) return -1;
    int i=buscar(s); if (i<0) return -2;
    if (capacidad<tabla[i].n || (!datos && tabla[i].n)) return -3;
    memcpy(datos,tabla[i].datos,tabla[i].n); return (int)tabla[i].n;
}
int nxk_archivo_agregar(const char *s,const void *datos,size_t n) {
    if (!valido(s) || (!datos && n)) return -1;
    int i=buscar(s); if (i<0) return -2;
    /* Restar evita que n + longitud desborde size_t. */
    if (n>256-tabla[i].n) return -3;
    if (n) memmove(tabla[i].datos+tabla[i].n,datos,n);
    tabla[i].n+=n; return 0;
}
int nxk_archivo_renombrar(const char *s,const char *nuevo) {
    if (!valido(s) || !valido(nuevo)) return -1;
    int i=buscar(s); if (i<0) return -2;
    int otro=buscar(nuevo);
    if (otro>=0) return otro==i ? 0 : -3;
    char copia[24]; unsigned j=0;
    do { copia[j]=nuevo[j]; } while (nuevo[j++]);
    memset(tabla[i].nombre,0,24); memcpy(tabla[i].nombre,copia,j);
    return 0;
}
int nxk_archivo_leer_visible(const char *nombre,void *datos,size_t capacidad) {
    int resultado=nxk_archivo_leer(nombre,datos,capacidad);
    /* No esconder errores de capacidad o nombres invalidos tras otro backend. */
    return resultado==-2 ? nxk_volumen_leer(nombre,datos,capacidad) : resultado;
}
int nxk_archivo_borrar(const char *s) {
    if (!valido(s)) return -1;
    int i=buscar(s); if (i<0) return -2;
    memset(&tabla[i],0,sizeof(tabla[i])); return 0;
}
const char *nxk_archivo_nombre(unsigned i) {
    return tabla && i<8 && tabla[i].nombre[0] ? tabla[i].nombre : 0;
}
int nxk_archivos_probar(void) {
    unsigned char b[256],d[256];
    for (unsigned i=0;i<256;++i) d[i]=(unsigned char)i;
    if (nxk_archivo_escribir("prueba",d,256) || nxk_archivo_escribir("prueba",d,257)!=-1 ||
        nxk_archivo_leer_visible("prueba",b,255)!=-3 || nxk_archivo_leer("prueba",b,256)!=256) return 1;
    for (unsigned i=0;i<256;++i) if (b[i]!=d[i]) return 2;
    if (nxk_archivo_borrar("prueba") || nxk_archivo_leer("prueba",b,256)!=-2) return 3;
    for (unsigned i=0;i<8;++i) { char s[2]={(char)('a'+i),0}; if (nxk_archivo_escribir(s,d,1)) return 4; }
    if (nxk_archivo_escribir("lleno",d,1)!=-3 || nxk_archivo_escribir("../x",d,1)!=-1) return 5;
    if (nxk_archivo_borrar("a") || nxk_archivo_escribir("nuevo",0,0) || nxk_archivo_leer("nuevo",b,256)!=0) return 6;
    if (nxk_archivo_agregar("nuevo",d,255) || nxk_archivo_agregar("nuevo",d,1) ||
        nxk_archivo_agregar("nuevo",d,(size_t)-1)!=-3 || nxk_archivo_leer("nuevo",b,256)!=256) return 7;
    for (unsigned i=0;i<256;++i) if (b[i]!=d[i%255]) return 8;
    if (nxk_archivo_renombrar("nuevo","b")!=-3 || nxk_archivo_renombrar("nuevo","renombrado") ||
        nxk_archivo_leer("nuevo",b,256)!=-2 || nxk_archivo_leer("renombrado",b,256)!=256 ||
        nxk_archivo_renombrar("renombrado","renombrado")) return 9;
    nxk_archivo_borrar("renombrado");
    for (unsigned i=1;i<8;++i) { char s[2]={(char)('a'+i),0}; nxk_archivo_borrar(s); }
    return 0;
}
