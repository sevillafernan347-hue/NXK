/* Formato original NXFS1, sin escritura. Metadatos y contenidos con FNV1a.
 * FNV detecta corrupcion accidental; no autentica archivos ni firma codigo. */
#include "volumen.h"
#include "ata.h"
#define FNV_INICIO 2166136261u
struct entrada { char nombre[24]; uint32_t longitud,lba,huella; };
static struct entrada archivos[8];
static unsigned cantidad;
static int montado;
static unsigned char *temporal;
/* El mismo buffer es una copia verificada solo para el siguiente tramo.
 * Este estado nunca reemplaza la validacion del nombre/rango del llamador. */
static const struct entrada *serie;
static uint32_t siguiente,revision_serie;
#if defined(__i386__) || defined(NXK_VOLUMEN_PRUEBA_HOST)
static uint32_t generacion_montada;
static struct {
    struct nxk_volumen_tarea progreso;
    uint32_t lba,esperada,huella,revision;
} tarea;
/* Una reserva adicional reutilizable. La ruta sincrona conserva su copia. */
static struct {
    uint32_t id,lba,revision,avance,huella,siguiente;
    unsigned char *datos;
} lector;
#endif
static uint32_t u32(const unsigned char *p) { return p[0]|(uint32_t)p[1]<<8|(uint32_t)p[2]<<16|(uint32_t)p[3]<<24; }
static uint32_t acumular_huella(uint32_t h,const unsigned char *p,size_t n) {
    while (n--) h=(h^*p++)*16777619u;
    return h;
}
static uint32_t huella(const unsigned char *p,size_t n) {
    return acumular_huella(FNV_INICIO,p,n);
}
static int todo_cero(const unsigned char *p,size_t n) {
    while (n--) if (*p++) return 0;
    return 1;
}
void nxk_volumen_desmontar(void) {
#if defined(__i386__) || defined(NXK_VOLUMEN_PRUEBA_HOST)
    if (tarea.progreso.estado==NXK_VOLUMEN_TAREA_PENDIENTE)
        tarea.progreso.estado=NXK_VOLUMEN_TAREA_INVALIDADA;
    lector.id=0;
#endif
    /* El buffer del heap fijo se reutiliza, pero sus bytes ya no son validos. */
    serie=0;
    cantidad=0;
    montado=0;
}

static void comprobar_generacion(void) {
#if defined(__i386__) || defined(NXK_VOLUMEN_PRUEBA_HOST)
    if (montado && generacion_montada!=nxk_ata_generacion_canal())
        nxk_volumen_desmontar();
#endif
}

int nxk_volumen_estado(void) {
    comprobar_generacion();
    return montado ? (int)cantidad : -1;
}

int nxk_volumen_montar(void) {
    /* Un fallo de montaje tambien retira el catalogo anterior. */
    nxk_volumen_desmontar();
#if defined(__i386__) || defined(NXK_VOLUMEN_PRUEBA_HOST)
    uint32_t generacion=nxk_ata_generacion_canal();
    if (generacion==UINT32_MAX) return -1;
#endif
    struct nxk_ata_info info; unsigned char cabecera[512]; struct entrada nuevas[8]={0};
    if (nxk_ata_identificar(1,&info) || nxk_ata_leer_sector(1,0,cabecera)) return -1;
    if (u32(cabecera)!=0x5346584e || u32(cabecera+4)!=1 ||
        u32(cabecera+8)>8 || huella(cabecera,508)!=u32(cabecera+508)) return -2;
    unsigned n=u32(cabecera+8);
    /* Los campos reservados y las entradas inactivas no tienen significado
     * en v1. Exigir ceros preserva una sola representacion de la cabecera. */
    unsigned fin_tabla=16+n*40;
    if (u32(cabecera+12) || !todo_cero(cabecera+fin_tabla,508-fin_tabla)) return -2;
    for (unsigned i=0;i<n;++i) {
        const unsigned char *p=cabecera+16+i*40; unsigned fin=24;
        for (unsigned j=0;j<24;++j) {
            unsigned char c=p[j];
            if (!c) { fin=j; break; }
            if (!((c>='a' && c<='z') || (c>='0' && c<='9') || c=='.' || c=='_' || c=='-')) return -3;
        }
        if (!fin || fin==24) return -3;
        if (!todo_cero(p+fin+1,23-fin) || u32(p+36)) return -2;
        memcpy(nuevas[i].nombre,p,fin+1);
        nuevas[i].longitud=u32(p+24); nuevas[i].lba=u32(p+28); nuevas[i].huella=u32(p+32);
        uint32_t sectores=(nuevas[i].longitud+511)/512;
        if (!sectores) sectores=1;
        if (nuevas[i].longitud>4096 || !nuevas[i].lba || nuevas[i].lba>=info.sectores || sectores>info.sectores-nuevas[i].lba) return -4;
        for (unsigned j=0;j<i;++j) {
            uint32_t prev=(nuevas[j].longitud+511)/512; if (!prev) prev=1;
            if (nxk_equal(nuevas[i].nombre,nuevas[j].nombre) ||
                (nuevas[i].lba<nuevas[j].lba+prev && nuevas[j].lba<nuevas[i].lba+sectores)) return -5;
        }
    }
    memcpy(archivos,nuevas,n*sizeof(*archivos));
    cantidad=n;
#if defined(__i386__) || defined(NXK_VOLUMEN_PRUEBA_HOST)
    generacion_montada=generacion;
#endif
    montado=1;
    return 0;
}
const char *nxk_volumen_nombre(unsigned i) {
    comprobar_generacion();
    return i<cantidad ? archivos[i].nombre : 0;
}
static const struct entrada *buscar(const char *nombre) {
    comprobar_generacion();
    for (unsigned i=0;i<cantidad;++i)
        if (nxk_equal(nombre,archivos[i].nombre)) return &archivos[i];
    return 0;
}

#if defined(__i386__) || defined(NXK_VOLUMEN_PRUEBA_HOST)
static void comprobar_tarea(void) {
    if (tarea.progreso.estado!=NXK_VOLUMEN_TAREA_PENDIENTE) return;
    comprobar_generacion();
    if (tarea.revision!=nxk_ata_revision_esclavo())
        tarea.progreso.estado=NXK_VOLUMEN_TAREA_INVALIDADA;
}

static void terminar_tarea(void) {
    tarea.progreso.estado=tarea.huella==tarea.esperada ?
        NXK_VOLUMEN_TAREA_VERIFICADA : NXK_VOLUMEN_HUELLA_INVALIDA;
}

int nxk_volumen_tarea_iniciar(const char *nombre) {
    if (!nombre) return -1;
    comprobar_tarea();
    if (tarea.progreso.estado==NXK_VOLUMEN_TAREA_PENDIENTE) return -3;
    const struct entrada *a=buscar(nombre);
    if (!a) return NXK_VOLUMEN_SIN_ENTRADA;
    uint32_t revision=nxk_ata_revision_esclavo();
    if (a->longitud && revision==UINT32_MAX) return NXK_VOLUMEN_TAREA_INVALIDADA;
    tarea.progreso=(struct nxk_volumen_tarea){0,a->longitud,NXK_VOLUMEN_TAREA_PENDIENTE};
    tarea.lba=a->lba;
    tarea.esperada=a->huella;
    tarea.huella=FNV_INICIO;
    tarea.revision=revision;
    if (!a->longitud) terminar_tarea();
    return 0;
}

void nxk_volumen_tarea_paso(void) {
    comprobar_tarea();
    if (tarea.progreso.estado!=NXK_VOLUMEN_TAREA_PENDIENTE) return;
    /* El sector vive en esta llamada. La copia verificada de las lecturas
     * sincronas sigue intacta aunque el sector de esta tarea falle. */
    unsigned char sector[512];
    uint32_t pos=tarea.progreso.leidos;
    if (nxk_ata_leer_sector(1,tarea.lba+pos/512,sector)) {
        tarea.progreso.estado=NXK_VOLUMEN_ERROR_ATA;
        return;
    }
    uint32_t n=tarea.progreso.total-pos;
    if (n>512) n=512;
    tarea.huella=acumular_huella(tarea.huella,sector,n);
    tarea.progreso.leidos+=n;
    if (tarea.progreso.leidos==tarea.progreso.total) terminar_tarea();
}

void nxk_volumen_tarea_cancelar(void) {
    comprobar_tarea();
    if (tarea.progreso.estado==NXK_VOLUMEN_TAREA_PENDIENTE)
        tarea.progreso.estado=NXK_VOLUMEN_TAREA_CANCELADA;
}

struct nxk_volumen_tarea nxk_volumen_tarea_consultar(void) {
    comprobar_tarea();
    return tarea.progreso;
}

int nxk_volumen_lectura_preparar(const char *nombre,struct nxk_volumen_lectura *salida) {
    const struct entrada *a=buscar(nombre);
    uint32_t revision=nxk_ata_revision_esclavo();
    if (!a || revision==UINT32_MAX) return -1;
    *salida=(struct nxk_volumen_lectura){a->lba,a->longitud,a->huella,revision};
    return 0;
}

void nxk_volumen_lectura_olvidar(uint32_t id) {
    if (lector.id==id) lector.id=0;
}

int nxk_volumen_lectura_paso(uint32_t id,const struct nxk_volumen_lectura *a,
                            uint32_t posicion,uint32_t capacidad,void *salida) {
    comprobar_generacion();
    if (!montado || a->revision!=nxk_ata_revision_esclavo() ||
        posicion>=a->longitud || a->longitud>4096 || !capacidad || capacidad>256) {
        nxk_volumen_lectura_olvidar(id);
        return -1;
    }
    if (!lector.datos) lector.datos=nxk_alloc(4096);
    if (!lector.datos) return -1;
    if (lector.id!=id || lector.lba!=a->lba || lector.revision!=a->revision ||
        (lector.avance==a->longitud && (!posicion || posicion!=lector.siguiente))) {
        lector.id=id;
        lector.lba=a->lba;
        lector.revision=a->revision;
        lector.avance=0;
        lector.huella=FNV_INICIO;
    }
    if (lector.avance<a->longitud) {
        unsigned char *sector=lector.datos+lector.avance;
        if (nxk_ata_leer_sector(1,a->lba+lector.avance/512,sector)) {
            lector.id=0;
            return -1;
        }
        uint32_t n=a->longitud-lector.avance;
        if (n>512) n=512;
        lector.huella=acumular_huella(lector.huella,sector,n);
        lector.avance+=n;
        if (lector.avance<a->longitud) return NXK_VOLUMEN_LECTURA_PENDIENTE;
        if (lector.huella!=a->huella) {
            lector.id=0;
            return -1;
        }
    }
    uint32_t n=a->longitud-posicion;
    if (n>capacidad) n=capacidad;
    memcpy(salida,lector.datos+posicion,n);
    lector.siguiente=posicion+n;
    if (lector.siguiente==a->longitud) lector.id=0;
    return (int)n;
}
#endif

int nxk_volumen_tamano(const char *nombre,uint32_t *salida) {
    if (!nombre || !salida) return -1;
    const struct entrada *a=buscar(nombre);
    if (!a) return -2;
    *salida=a->longitud;
    return 0;
}

/* El montaje limita la longitud a 4096. Incluso el ultimo sector de una
 * longitud no alineada cabe completo en el buffer. La huella cubre solo los
 * bytes del archivo. No publicar datos ante un fallo de disco o de huella. */
static int verificar(const struct entrada *a) {
    serie=0; /* Incluso una lectura fallida puede sobrescribir el buffer. */
    if (!a->longitud)
        return a->huella==FNV_INICIO ? 0 : NXK_VOLUMEN_HUELLA_INVALIDA;
    if (!temporal) temporal=nxk_alloc(4096);
    if (!temporal) return NXK_VOLUMEN_SIN_MEMORIA;
    for (uint32_t pos=0;pos<a->longitud;pos+=512)
        if (nxk_ata_leer_sector(1,a->lba+pos/512,temporal+pos))
            return NXK_VOLUMEN_ERROR_ATA;
    return huella(temporal,a->longitud)==a->huella ? 0 : NXK_VOLUMEN_HUELLA_INVALIDA;
}

int nxk_volumen_verificar(unsigned indice) {
    comprobar_generacion();
    if (indice>=cantidad) return NXK_VOLUMEN_SIN_ENTRADA;
    return verificar(&archivos[indice]);
}

static int leer_rango(const char *nombre,void *destino,size_t capacidad,uint32_t desplazamiento,int completo) {
    if (!nombre || (!destino && capacidad)) return -1;
    const struct entrada *a=buscar(nombre);
    if (!a) return -2;
    uint32_t n=a->longitud;
    if (desplazamiento>n || (completo && capacidad<n)) return -3;
    size_t copiar=n-desplazamiento; if (copiar>capacidad) copiar=capacidad;
    if (!copiar && n) return 0;
    if (!copiar) return verificar(a);
    uint32_t revision=nxk_ata_revision_esclavo();
    if (!desplazamiento || serie!=a || siguiente!=desplazamiento ||
        revision==UINT32_MAX || revision!=revision_serie) {
        int r=verificar(a);
        if (r) return r;
    }
    memcpy(destino,temporal+desplazamiento,copiar);
    siguiente=desplazamiento+(uint32_t)copiar;
    revision_serie=revision;
    serie=!completo && siguiente<n ? a : 0;
    return (int)copiar;
}

int nxk_volumen_visitar(const char *nombre,void (*visitar)(const unsigned char *,size_t)) {
    if (!nombre || !visitar) return -1;
    const struct entrada *a=buscar(nombre);
    if (!a) return -2;
    int r=verificar(a);
    if (r) return r;
    visitar(a->longitud ? temporal : 0,a->longitud);
    return 0;
}

int nxk_volumen_leer(const char *n,void *d,size_t c) { return leer_rango(n,d,c,0,1); }
int nxk_volumen_leer_parte(const char *n,void *d,size_t c,uint32_t p) { return leer_rango(n,d,c,p,0); }
