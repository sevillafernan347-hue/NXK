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
/* Rechazar antes de tocar datos. Limpiar tanto el tramo recortado como el
 * ampliado evita exponer contenido anterior al volver a extender un archivo. */
int nxk_archivo_truncar(const char *s,size_t longitud) {
    if (!valido(s)) return -1;
    int i=buscar(s); if (i<0) return -2;
    if (longitud>256) return -3;
    size_t anterior=tabla[i].n;
    if (longitud<anterior) memset(tabla[i].datos+longitud,0,anterior-longitud);
    else if (longitud>anterior) memset(tabla[i].datos+anterior,0,longitud-anterior);
    tabla[i].n=longitud; return 0;
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
int nxk_archivo_leer_parte(const char *nombre,void *datos,size_t capacidad,uint32_t desplazamiento) {
    if (!valido(nombre) || (!datos && capacidad)) return -1;
    int i=buscar(nombre);
    if (i<0) return nxk_volumen_leer_parte(nombre,datos,capacidad,desplazamiento);
    if (desplazamiento>tabla[i].n) return -3;
    size_t n=tabla[i].n-desplazamiento; if (n>capacidad) n=capacidad;
    if (n) memcpy(datos,tabla[i].datos+desplazamiento,n);
    return (int)n;
}
int nxk_archivo_leer_visible(const char *nombre,void *datos,size_t capacidad) {
    int resultado=nxk_archivo_leer(nombre,datos,capacidad);
    /* No esconder errores de capacidad o nombres invalidos tras otro backend. */
    return resultado==-2 ? nxk_volumen_leer(nombre,datos,capacidad) : resultado;
}
/* Copia visible a RAM sin reemplazar destinos, ni siquiera los del volumen.
 * El buffer local completa lectura y verificacion antes de crear la entrada.
 * Sin concurrencia: comprobacion y publicacion pertenecen a la misma llamada. */
int nxk_archivo_copiar(const char *origen,const char *destino) {
    unsigned char datos[256], comprobacion[256];
    if (!valido(origen) || !valido(destino)) return -1;
    int existente=nxk_archivo_leer_visible(destino,comprobacion,sizeof(comprobacion));
    /* Solo ausencia permite publicar. Errores y destinos grandes se conservan. */
    if (existente!=-2) return -3;
    int n=nxk_archivo_leer_visible(origen,datos,sizeof(datos));
    if (n<0) return n;
    return nxk_archivo_escribir(destino,datos,(size_t)n);
}
int nxk_archivo_borrar(const char *s) {
    if (!valido(s)) return -1;
    int i=buscar(s); if (i<0) return -2;
    memset(&tabla[i],0,sizeof(tabla[i])); return 0;
}
const char *nxk_archivo_nombre(unsigned i) {
    return tabla && i<8 && tabla[i].nombre[0] ? tabla[i].nombre : 0;
}
/* La etapa estatica evita consumir mas de media pila de una tarea.
 * No ceder ni usar desde IRQ: prepara una imagen completa antes de publicarla. */
static struct nxk_archivo_imagen imagen_preparada[8];
int nxk_archivos_exportar(struct nxk_archivo_imagen *salida) {
    if (!salida) return -1;
    memset(salida,0,8*sizeof(*salida));
    if (tabla) for (unsigned i=0;i<8;++i) if (tabla[i].nombre[0]) {
        unsigned j=0; do { salida[i].nombre[j]=tabla[i].nombre[j]; } while (tabla[i].nombre[j++]);
        memcpy(salida[i].datos,tabla[i].datos,tabla[i].n);
        salida[i].longitud=(uint32_t)tabla[i].n;
    }
    return 0;
}
int nxk_archivos_importar(const struct nxk_archivo_imagen *entrada) {
    if (!entrada) return -1;
    /* Copiar primero permite validar bytes estables, incluso si el origen
     * proviene de la memoria que se reemplazara. No asignar antes de validar. */
    memmove(imagen_preparada,entrada,sizeof(imagen_preparada));
    unsigned usados=0;
    for (unsigned i=0;i<8;++i) {
        const struct nxk_archivo_imagen *a=&imagen_preparada[i];
        if (!a->nombre[0]) {
            const unsigned char *bytes=(const void *)a;
            for (unsigned j=0;j<sizeof(*a);++j) if (bytes[j]) return -2;
            continue;
        }
        if (!valido(a->nombre) || a->longitud>256) return -2;
        unsigned fin=0; while (a->nombre[fin]) ++fin;
        for (unsigned j=fin+1;j<24;++j) if (a->nombre[j]) return -2;
        for (unsigned j=a->longitud;j<256;++j) if (a->datos[j]) return -2;
        for (unsigned j=0;j<i;++j)
            if (nxk_equal(a->nombre,imagen_preparada[j].nombre)) return -2;
        ++usados;
    }
    /* Una imagen vacia no necesita crear la tabla. Ya no puede fallar
     * ninguna operacion despues de reservarla o de empezar a reemplazarla. */
    if (!tabla && usados) { tabla=nxk_alloc(8*sizeof(*tabla)); if (!tabla) return -3; }
    if (tabla) {
        memset(tabla,0,8*sizeof(*tabla));
        for (unsigned i=0;i<8;++i) {
            memcpy(tabla[i].nombre,imagen_preparada[i].nombre,24);
            memcpy(tabla[i].datos,imagen_preparada[i].datos,256);
            tabla[i].n=imagen_preparada[i].longitud;
        }
    }
    return 0;
}
#ifdef NXK_TEST
/* Buffers de prueba estaticos: no forzar las pilas pequenas del kernel. */
static struct nxk_archivo_imagen prueba_imagen[8], prueba_salida[8];
static int probar_imagen(void) {
    size_t usado=nxk_heap_used();
    memset(prueba_imagen,0,sizeof(prueba_imagen));
    if (nxk_archivos_exportar(0)!=-1 || nxk_archivos_importar(0)!=-1 ||
        nxk_archivos_importar(prueba_imagen) || nxk_archivos_exportar(prueba_salida)) return 1;
    for (unsigned i=0;i<sizeof(prueba_salida);++i)
        if (((unsigned char *)prueba_salida)[i]) return 2;
    for (unsigned i=0;i<8;++i) {
        prueba_imagen[i].nombre[0]=(char)('a'+i);
        prueba_imagen[i].longitud=i ? 256 : 0;
        if (i) for (unsigned j=0;j<256;++j) prueba_imagen[i].datos[j]=(unsigned char)(i+j);
    }
    if (nxk_archivos_importar(prueba_imagen) || nxk_archivo_escribir("noveno",0,0)!=-3 ||
        nxk_archivos_exportar(prueba_salida)) return 3;
    for (unsigned i=0;i<sizeof(prueba_salida);++i)
        if (((unsigned char *)prueba_salida)[i]!=((unsigned char *)prueba_imagen)[i]) return 4;
    /* Cada error debe conservar los ocho archivos y no reservar memoria. */
    for (unsigned caso=0;caso<6;++caso) {
        memcpy(prueba_imagen,prueba_salida,sizeof(prueba_imagen));
        if (caso==0) prueba_imagen[7].nombre[0]='a'; /* Duplicado al final. */
        if (caso==1) prueba_imagen[7].longitud=257;
        if (caso==2) prueba_imagen[7].nombre[2]='x'; /* Nombre con relleno sucio. */
        if (caso==3) prueba_imagen[0].datos[0]=1; /* Datos fuera de longitud. */
        if (caso==4) prueba_imagen[7].nombre[0]=0; /* Entrada vacia sucia. */
        if (caso==5) memset(prueba_imagen[7].nombre,'x',24); /* Sin NUL. */
        if (nxk_archivos_importar(prueba_imagen)!=-2 || nxk_archivos_exportar(prueba_imagen)) return 5;
        for (unsigned i=0;i<sizeof(prueba_salida);++i)
            if (((unsigned char *)prueba_salida)[i]!=((unsigned char *)prueba_imagen)[i]) return 6;
    }
    for (unsigned i=0;i<3;++i)
        if (nxk_archivos_importar(prueba_salida) || nxk_heap_used()!=usado) return 7;
    memset(prueba_imagen,0,sizeof(prueba_imagen));
    if (nxk_archivos_importar(prueba_imagen) || nxk_archivos_exportar(prueba_salida)) return 8;
    for (unsigned i=0;i<sizeof(prueba_salida);++i)
        if (((unsigned char *)prueba_salida)[i]) return 9;
    return nxk_heap_used()!=usado ? 10 : 0;
}
#endif
int nxk_archivos_probar(void) {
    unsigned char b[256],d[256];
    for (unsigned i=0;i<256;++i) d[i]=(unsigned char)i;
    if (nxk_archivo_escribir("prueba",d,256) || nxk_archivo_escribir("prueba",d,257)!=-1 ||
        nxk_archivo_leer_visible("prueba",b,255)!=-3 || nxk_archivo_leer("prueba",b,256)!=256) return 1;
    for (unsigned i=0;i<256;++i) if (b[i]!=d[i]) return 2;
    if (nxk_archivo_leer_parte("prueba",b,5,254)!=2 || b[0]!=254 || b[1]!=255 ||
        nxk_archivo_leer_parte("prueba",b,256,256)!=0 ||
        nxk_archivo_leer_parte("prueba",b,256,257)!=-3 ||
        nxk_archivo_leer_parte("prueba",b,256,UINT32_MAX)!=-3) return 10;
    if (nxk_archivo_borrar("prueba") || nxk_archivo_leer("prueba",b,256)!=-2) return 3;
    for (unsigned i=0;i<8;++i) { char s[2]={(char)('a'+i),0}; if (nxk_archivo_escribir(s,d,1)) return 4; }
    if (nxk_archivo_copiar("b","copia")!=-3 || nxk_archivo_leer("copia",b,256)!=-2) return 11;
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
    if (nxk_archivo_escribir("original",d,256) || nxk_archivo_copiar("original","copia") ||
        nxk_archivo_copiar("original","copia")!=-3 || nxk_archivo_copiar("original","original")!=-3 ||
        nxk_archivo_copiar("ausente","nuevo")!=-2 || nxk_archivo_leer("nuevo",b,256)!=-2 ||
        nxk_archivo_copiar("original","../x")!=-1 || nxk_archivo_leer("copia",b,256)!=256) return 12;
    for (unsigned i=0;i<256;++i) if (b[i]!=d[i]) return 13;
    if (nxk_archivo_borrar("original") || nxk_archivo_leer("copia",b,256)!=256) return 14;
    nxk_archivo_borrar("copia");
    if (nxk_archivo_escribir("vacio",0,0) || nxk_archivo_copiar("vacio","copia") ||
        nxk_archivo_leer("copia",b,256)!=0) return 15;
    nxk_archivo_borrar("vacio"); nxk_archivo_borrar("copia");
    /* Recortar y ampliar preserva el prefijo y rellena lo nuevo con cero. */
    if (nxk_archivo_truncar("no-existe",2)!=-2 || nxk_archivo_truncar("../x",2)!=-1 ||
        nxk_archivo_escribir("tamano",d,256) || nxk_archivo_truncar("tamano",257)!=-3 ||
        nxk_archivo_truncar("tamano",(size_t)-1)!=-3 || nxk_archivo_leer("tamano",b,256)!=256) return 16;
    for (unsigned i=0;i<256;++i) if (b[i]!=d[i]) return 17;
    if (nxk_archivo_truncar("tamano",7) || nxk_archivo_truncar("tamano",7) ||
        nxk_archivo_truncar("tamano",256) || nxk_archivo_leer("tamano",b,256)!=256) return 18;
    for (unsigned i=0;i<256;++i) if (b[i]!=(i<7 ? d[i] : 0)) return 19;
    if (nxk_archivo_truncar("tamano",0) || nxk_archivo_leer("tamano",0,0)!=0 ||
        nxk_archivo_truncar("tamano",4) || nxk_archivo_leer("tamano",b,256)!=4 ||
        b[0] || b[1] || b[2] || b[3] || nxk_archivo_borrar("tamano")) return 20;
#ifdef NXK_TEST
    int imagen=probar_imagen(); if (imagen) return 20+imagen;
#endif
    return 0;
}
