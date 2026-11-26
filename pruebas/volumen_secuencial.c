/* Ejecuta el lector NXFS real con un disco simulado en memoria. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "ata.h"
#include "volumen.h"

static unsigned char disco[10][512], temporal[4096], diferido[4096];
static unsigned lecturas, reservas, visitas;
static int fallar_reserva,pausar_lectura;
static unsigned descartes;
static uint32_t revision, generacion, fallar_lba=UINT32_MAX;

int nxk_ata_identificar(unsigned unidad,struct nxk_ata_info *info) {
    assert(unidad==1);
    info->sectores=10;
    if (revision!=UINT32_MAX) ++revision;
    return 0;
}
uint32_t nxk_ata_revision_esclavo(void) { return revision; }
uint32_t nxk_ata_generacion_canal(void) { return generacion; }
int nxk_ata_leer_sector(unsigned unidad,uint32_t lba,void *destino) {
    assert(unidad==1 && lba<10);
    ++lecturas;
    if (lba==fallar_lba) return -3;
    memcpy(destino,disco[lba],512);
    return 0;
}
int nxk_ata_lectura_paso(unsigned unidad,uint32_t lba,void *destino) {
    if (pausar_lectura) return NXK_ATA_LECTURA_PENDIENTE;
    return nxk_ata_leer_sector(unidad,lba,destino);
}
void nxk_ata_lectura_descartar(void) { ++descartes; }
void *nxk_alloc(size_t bytes) {
    assert(bytes==sizeof(temporal) && reservas<2);
    if (fallar_reserva) return 0;
    return reservas++ ? diferido : temporal;
}
int nxk_equal(const char *a,const char *b) { return strcmp(a,b)==0; }

static void ver_otro(const unsigned char *datos,size_t n) {
    assert(n==5 && !memcmp(datos,"texto",5));
    ++visitas;
}

static void verificar_progreso(int estado,uint32_t leidos,uint32_t total) {
    struct nxk_volumen_tarea p=nxk_volumen_tarea_consultar();
    assert(p.estado==estado && p.leidos==leidos && p.total==total);
}

static void probar_tarea(void) {
    unsigned char salida[256];
    generacion=0;
    revision=10;
    assert(!nxk_volumen_montar());
    lecturas=0;
    verificar_progreso(NXK_VOLUMEN_TAREA_INACTIVA,0,0);
    assert(nxk_volumen_tarea_iniciar(0)==-1);
    assert(nxk_volumen_tarea_iniciar("ausente")==NXK_VOLUMEN_SIN_ENTRADA && !lecturas);
    assert(nxk_volumen_leer_parte("a",salida,sizeof(salida),0)==256 && lecturas==8);
    assert(!nxk_volumen_tarea_iniciar("a") && lecturas==8);
    assert(nxk_volumen_tarea_iniciar("b")==-3);
    verificar_progreso(NXK_VOLUMEN_TAREA_PENDIENTE,0,4096);
    nxk_volumen_tarea_paso();
    verificar_progreso(NXK_VOLUMEN_TAREA_PENDIENTE,512,4096);
    assert(lecturas==9);
    /* La verificacion en curso no invalida la copia de lectura contigua. */
    assert(nxk_volumen_leer_parte("a",salida,sizeof(salida),256)==256 && lecturas==9);
    for (unsigned i=1;i<8;++i) {
        nxk_volumen_tarea_paso();
        verificar_progreso(i==7 ? NXK_VOLUMEN_TAREA_VERIFICADA : NXK_VOLUMEN_TAREA_PENDIENTE,
                           (i+1)*512,4096);
        assert(lecturas==9+i);
    }
    nxk_volumen_tarea_paso();
    assert(lecturas==16 && reservas==1);
    assert(!nxk_volumen_tarea_iniciar("b"));
    nxk_volumen_tarea_paso();
    verificar_progreso(NXK_VOLUMEN_TAREA_VERIFICADA,5,5);
    assert(lecturas==17);

    assert(!nxk_volumen_tarea_iniciar("a"));
    nxk_volumen_tarea_paso();
    nxk_volumen_tarea_cancelar();
    nxk_volumen_tarea_paso();
    verificar_progreso(NXK_VOLUMEN_TAREA_CANCELADA,512,4096);
    assert(lecturas==18);

    assert(!nxk_volumen_tarea_iniciar("a"));
    nxk_volumen_tarea_paso();
    ++revision;
    nxk_volumen_tarea_paso();
    verificar_progreso(NXK_VOLUMEN_TAREA_INVALIDADA,512,4096);
    assert(lecturas==19);

    assert(!nxk_volumen_tarea_iniciar("b"));
    ++generacion;
    nxk_volumen_tarea_paso();
    verificar_progreso(NXK_VOLUMEN_TAREA_INVALIDADA,0,5);
    assert(lecturas==19 && nxk_volumen_estado()==-1);
    assert(!nxk_volumen_montar());
    assert(!nxk_volumen_tarea_iniciar("b"));
    nxk_volumen_desmontar();
    assert(!nxk_volumen_montar());
    verificar_progreso(NXK_VOLUMEN_TAREA_INVALIDADA,0,5);
    assert(lecturas==21);

    assert(!nxk_volumen_tarea_iniciar("b"));
    fallar_lba=9;
    nxk_volumen_tarea_paso();
    verificar_progreso(NXK_VOLUMEN_ERROR_ATA,0,5);
    fallar_lba=UINT32_MAX;
    disco[9][0]^=1;
    assert(!nxk_volumen_tarea_iniciar("b"));
    nxk_volumen_tarea_paso();
    verificar_progreso(NXK_VOLUMEN_HUELLA_INVALIDA,5,5);
    disco[9][0]^=1;
    revision=UINT32_MAX;
    assert(nxk_volumen_tarea_iniciar("b")==NXK_VOLUMEN_TAREA_INVALIDADA);
    assert(lecturas==23 && reservas==1);
    puts("NXFS HOST OK: tarea por sectores, coexistencia con lectura contigua, cancelacion y cambios de disco.");
}

static void probar_lectura_diferida(void) {
    struct nxk_volumen_lectura a;
    unsigned char salida[256];
    revision=10;
    assert(!nxk_volumen_montar() && !nxk_volumen_lectura_preparar("a",&a));
    lecturas=0;
    memset(salida,0xa5,sizeof(salida));
    fallar_reserva=1;
    assert(nxk_volumen_lectura_paso(257,&a,0,256,salida)==-1 && !lecturas && reservas==1);
    fallar_reserva=0;
    pausar_lectura=1;
    unsigned antes=descartes;
    assert(nxk_volumen_lectura_paso(257,&a,0,256,salida)==NXK_VOLUMEN_LECTURA_PENDIENTE && !lecturas);
    nxk_volumen_lectura_olvidar(257);
    assert(descartes==antes+1);
    for (unsigned j=0;j<sizeof(salida);++j) assert(salida[j]==0xa5);
    pausar_lectura=0;
    for (unsigned i=0;i<7;++i) {
        assert(nxk_volumen_lectura_paso(257,&a,0,256,salida)==NXK_VOLUMEN_LECTURA_PENDIENTE);
        assert(lecturas==i+1);
        for (unsigned j=0;j<sizeof(salida);++j) assert(salida[j]==0xa5);
    }
    assert(nxk_volumen_lectura_paso(257,&a,0,256,salida)==256 && lecturas==8);
    for (unsigned p=256;p<4096;p+=256) {
        assert(nxk_volumen_lectura_paso(257,&a,p,256,salida)==256 && lecturas==8);
        for (unsigned i=0;i<sizeof(salida);++i) assert(salida[i]==(p+i)%251);
    }
    /* Otro ID no usa una copia verificada para el solicitante anterior. */
    for (unsigned i=0;i<7;++i)
        assert(nxk_volumen_lectura_paso(257,&a,0,256,salida)==NXK_VOLUMEN_LECTURA_PENDIENTE);
    assert(nxk_volumen_lectura_paso(257,&a,0,256,salida)==256 && lecturas==16);
    assert(nxk_volumen_lectura_paso(513,&a,256,256,salida)==NXK_VOLUMEN_LECTURA_PENDIENTE);
    assert(lecturas==17 && reservas==2);
    nxk_volumen_lectura_olvidar(513);
    assert(nxk_volumen_lectura_paso(513,&a,256,256,salida)==NXK_VOLUMEN_LECTURA_PENDIENTE);
    ++revision;
    assert(nxk_volumen_lectura_paso(513,&a,256,256,salida)==-1 && lecturas==18);

    assert(!nxk_volumen_lectura_preparar("a",&a));
    disco[8][511]^=1;
    memset(salida,0xa5,sizeof(salida));
    for (unsigned i=0;i<7;++i)
        assert(nxk_volumen_lectura_paso(257,&a,510,16,salida)==NXK_VOLUMEN_LECTURA_PENDIENTE);
    assert(nxk_volumen_lectura_paso(257,&a,510,16,salida)==-1 && lecturas==26);
    for (unsigned i=0;i<sizeof(salida);++i) assert(salida[i]==0xa5);
    disco[8][511]^=1;
    puts("NXFS HOST OK: lectura diferida, ocho sectores para 4 KiB, aislamiento y error final sin copia.");
}

int main(int argc,char **argv) {
    assert(argc==2);
    FILE *archivo=fopen(argv[1],"rb");
    assert(archivo && fread(disco,1,sizeof(disco),archivo)==sizeof(disco));
    assert(!fclose(archivo));
    assert(nxk_volumen_estado()==-1 && !lecturas && !revision);
    assert(nxk_volumen_verificar(0)==NXK_VOLUMEN_SIN_ENTRADA && !lecturas);
    assert(!nxk_volumen_montar() && nxk_volumen_estado()==2);
    lecturas=0;
    fallar_reserva=1;
    assert(nxk_volumen_verificar(0)==NXK_VOLUMEN_SIN_MEMORIA);
    assert(!lecturas && !reservas);
    fallar_reserva=0;
    unsigned char salida[258];
    memset(salida,0xa5,sizeof(salida));
    for (uint32_t p=0;p<4096;p+=256) {
        assert(nxk_volumen_leer_parte("a",salida+1,256,p)==256);
        for (unsigned i=0;i<256;++i)
            assert(salida[i+1]==(unsigned char)((p+i)%251));
    }
    assert(lecturas==8 && reservas==1);
    assert(salida[0]==0xa5 && salida[257]==0xa5);
    assert(!nxk_volumen_leer_parte("a",salida+1,256,4096) && lecturas==8);

    assert(nxk_volumen_leer_parte("a",salida+1,256,0)==256 && lecturas==16);
    assert(nxk_volumen_leer_parte("a",salida+1,256,768)==256 && lecturas==24);
    assert(!nxk_volumen_visitar("b",ver_otro) && visitas==1 && lecturas==25);
    assert(nxk_volumen_leer_parte("a",salida+1,256,1024)==256 && lecturas==33);

    /* Cambiar la revision invalida los bytes aunque se pida el tramo esperado. */
    ++revision;
    disco[8][511]^=1;
    memset(salida,0xa5,sizeof(salida));
    assert(nxk_volumen_leer_parte("a",salida+1,256,1280)==-6 && lecturas==41);
    for (unsigned i=0;i<sizeof(salida);++i) assert(salida[i]==0xa5);
    disco[8][511]^=1;
    assert(nxk_volumen_leer_parte("a",salida+1,256,1280)==256 && lecturas==49);

    /* Un intento fallido tras sobreescribir siete sectores no publica cache. */
    fallar_lba=8;
    assert(nxk_volumen_leer_parte("a",salida+1,256,0)==-5 && lecturas==57);
    fallar_lba=UINT32_MAX;
    assert(nxk_volumen_leer_parte("a",salida+1,256,1536)==256 && lecturas==65);
    assert(!nxk_volumen_montar() && lecturas==66);
    assert(nxk_volumen_leer_parte("a",salida+1,256,1792)==256 && lecturas==74);

    revision=UINT32_MAX;
    assert(nxk_volumen_leer_parte("a",salida+1,256,0)==256 && lecturas==82);
    assert(nxk_volumen_leer_parte("a",salida+1,256,256)==256 && lecturas==90);
    puts("NXFS HOST OK: 16 tramos, 8 sectores, cambios de revision y recuperacion.");

    /* Crear una serie activa y desmontar sin lecturas ni nueva identificacion. */
    revision=10;
    assert(nxk_volumen_leer_parte("a",salida+1,256,0)==256 && lecturas==98);
    nxk_volumen_desmontar();
    nxk_volumen_desmontar();
    uint32_t tamano=99;
    memset(salida,0xa5,sizeof(salida));
    assert(nxk_volumen_estado()==-1 && !nxk_volumen_nombre(0));
    assert(nxk_volumen_tamano("a",&tamano)==-2 && tamano==99);
    assert(nxk_volumen_leer_parte("a",salida+1,256,256)==-2);
    assert(nxk_volumen_leer("b",salida,sizeof(salida))==-2);
    assert(nxk_volumen_visitar("b",ver_otro)==-2 && visitas==1);
    for (unsigned i=0;i<sizeof(salida);++i) assert(salida[i]==0xa5);
    assert(lecturas==98 && revision==10 && reservas==1);

    assert(!nxk_volumen_montar() && nxk_volumen_estado()==2 && lecturas==99);
    /* Simular que la revision coincide con la de la serie previa. El montaje
     * debe invalidarla por su cuenta, incluso si coincide el siguiente tramo. */
    revision=10;
    disco[8][511]^=1;
    assert(nxk_volumen_leer_parte("a",salida+1,256,256)==-6 && lecturas==107);
    disco[8][511]^=1;

    disco[0][508]^=1;
    assert(nxk_volumen_montar()==-2 && nxk_volumen_estado()==-1 && lecturas==108);
    assert(!nxk_volumen_nombre(0));
    assert(nxk_volumen_tamano("a",&tamano)==-2 && tamano==99 && lecturas==108);
    disco[0][508]^=1;
    assert(!nxk_volumen_montar() && nxk_volumen_estado()==2 && lecturas==109);
    assert(nxk_volumen_leer_parte("a",salida+1,256,256)==256 && lecturas==117);
    assert(reservas==1);
    puts("NXFS HOST OK: desmontaje, rechazo de cache previa y recuperacion tras montaje fallido.");

    /* Verificar el mismo archivo no debe aceptar la serie anterior. */
    lecturas=0;
    assert(nxk_volumen_verificar(2)==NXK_VOLUMEN_SIN_ENTRADA && !lecturas);
    assert(nxk_volumen_leer_parte("a",salida+1,256,0)==256 && lecturas==8);
    disco[8][511]^=1;
    assert(nxk_volumen_verificar(0)==NXK_VOLUMEN_HUELLA_INVALIDA && lecturas==16);
    assert(nxk_volumen_leer_parte("a",salida+1,256,256)==-6 && lecturas==24);
    disco[8][511]^=1;
    fallar_lba=9;
    assert(nxk_volumen_verificar(1)==NXK_VOLUMEN_ERROR_ATA && lecturas==25);
    fallar_lba=UINT32_MAX;
    assert(!nxk_volumen_verificar(1) && lecturas==26 && reservas==1);
    assert(nxk_volumen_estado()==2 && visitas==1);
    puts("NXFS HOST OK: verificacion directa, falta de memoria, error ATA y huella incorrecta.");

    assert(nxk_volumen_leer_parte("a",salida+1,256,0)==256 && lecturas==34);
    ++generacion;
    /* La primera consulta de metadatos tambien detecta el reinicio. */
    assert(nxk_volumen_tamano("a",&tamano)==-2 && tamano==99);
    assert(nxk_volumen_estado()==-1 && !nxk_volumen_nombre(0));
    assert(nxk_volumen_verificar(0)==NXK_VOLUMEN_SIN_ENTRADA);
    assert(nxk_volumen_leer_parte("a",salida+1,256,256)==-2 && lecturas==34);
    assert(!nxk_volumen_montar() && nxk_volumen_estado()==2 && lecturas==35);
    assert(nxk_volumen_leer_parte("a",salida+1,256,256)==256 && lecturas==43);
    generacion=UINT32_MAX;
    assert(nxk_volumen_estado()==-1 && nxk_volumen_montar()==-1 && lecturas==43);
    puts("NXFS HOST OK: reinicio de canal invalida catalogo y copia verificada.");
    probar_tarea();
    probar_lectura_diferida();
    return 0;
}
