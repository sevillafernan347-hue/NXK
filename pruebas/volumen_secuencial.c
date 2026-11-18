/* Ejecuta el lector NXFS real con un disco simulado en memoria. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "ata.h"
#include "volumen.h"

static unsigned char disco[10][512], temporal[4096];
static unsigned lecturas, reservas, visitas;
static uint32_t revision, fallar_lba=UINT32_MAX;

int nxk_ata_identificar(unsigned unidad,struct nxk_ata_info *info) {
    assert(unidad==1);
    info->sectores=10;
    if (revision!=UINT32_MAX) ++revision;
    return 0;
}
uint32_t nxk_ata_revision_esclavo(void) { return revision; }
int nxk_ata_leer_sector(unsigned unidad,uint32_t lba,void *destino) {
    assert(unidad==1 && lba<10);
    ++lecturas;
    if (lba==fallar_lba) return -3;
    memcpy(destino,disco[lba],512);
    return 0;
}
void *nxk_alloc(size_t bytes) {
    assert(bytes==sizeof(temporal) && !reservas);
    ++reservas;
    return temporal;
}
int nxk_equal(const char *a,const char *b) { return strcmp(a,b)==0; }

static void ver_otro(const unsigned char *datos,size_t n) {
    assert(n==5 && !memcmp(datos,"texto",5));
    ++visitas;
}

int main(int argc,char **argv) {
    assert(argc==2);
    FILE *archivo=fopen(argv[1],"rb");
    assert(archivo && fread(disco,1,sizeof(disco),archivo)==sizeof(disco));
    assert(!fclose(archivo));
    assert(!nxk_volumen_montar());
    lecturas=0;
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
    return 0;
}
