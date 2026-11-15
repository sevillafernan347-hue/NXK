/* Ejecuta el controlador real con puertos simulados. Nunca accede al hardware.
 * Cada fase consume una secuencia independiente; su ultimo estado se repite. */
#include <stdio.h>
#include <string.h>
#define NXK_ATA_PRUEBA_HOST 1
#include "../kernel/ata.c"
static struct {
    uint8_t estados[3][4],registro[8],comando;
    unsigned longitud[3],consultas[3],pausas[3],fase,lecturas,escrituras,comandos,io;
    uint16_t escritas[256];
} sim;
static unsigned casos,fallos;
static void comprobar(int condicion,const char *nombre) {
    ++casos;
    if (!condicion) { ++fallos; printf("FALLO: %s\n",nombre); }
}
static void secuencia(unsigned fase,uint8_t a,uint8_t b,uint8_t c) {
    sim.estados[fase][0]=a; sim.estados[fase][1]=b; sim.estados[fase][2]=c;
    sim.longitud[fase]=3;
}
static void reiniciar(void) {
    memset(&sim,0,sizeof(sim));
    secuencia(0,0x50,0x50,0x50);
    secuencia(1,0x58,0x58,0x58);
    secuencia(2,0x50,0x50,0x50);
}
static uint8_t entrada(uint16_t p) {
    ++sim.io;
    /* Contar las lecturas no certifica su duracion en hardware fisico. */
    if (p==0x3f6) { ++sim.pausas[sim.fase]; return 0x50; }
    if (p==0x1f7) {
        unsigned f=sim.fase,n=sim.consultas[f]++;
        if (n>=sim.longitud[f]) n=sim.longitud[f]-1;
        return sim.estados[f][n];
    }
    return p>=0x1f0 && p<=0x1f7 ? sim.registro[p-0x1f0] : 0;
}
static void salida(uint16_t p,uint8_t v) {
    ++sim.io;
    if (p>=0x1f0 && p<=0x1f7) sim.registro[p-0x1f0]=v;
    if (p==0x1f7) {
        sim.comando=v; ++sim.comandos;
        sim.fase=v==0xe7 ? 2 : 1;
    }
}
static uint16_t entrada_palabra(void) {
    ++sim.io;
    unsigned i=sim.lecturas++;
    if (sim.lecturas==256) sim.fase=2;
    if (sim.comando==0xec) {
        if (i==49) return 0x200;
        if (i==61) return 0x1000; /* Capacidad maxima LBA28. */
        return i>=27 && i<47 ? 0x2020 : 0;
    }
    return (uint16_t)(0x1200u+i);
}
static void salida_palabra(uint16_t v) {
    ++sim.io;
    if (sim.escrituras<256) sim.escritas[sim.escrituras]=v;
    if (++sim.escrituras==256) sim.fase=2;
}
static int centinela(const void *p,size_t n) {
    const unsigned char *b=p;
    for (size_t i=0;i<n;++i) if (b[i]!=0xa5) return 0;
    return 1;
}
int main(void) {
    struct nxk_ata_info info;
    unsigned char destino[514],origen[514];
    memset(origen,0x7b,sizeof(origen));
    for (unsigned u=0;u<2;++u) {
        reiniciar();
        comprobar(!nxk_ata_identificar(u,&info) && info.sectores==0x10000000u &&
                  sim.lecturas==256 && sim.pausas[2]==4,"IDENTIFY valido");
    }
    /* Los bytes altos de LBA28 no pueden perderse al seleccionar el esclavo. */
    reiniciar();
    comprobar(!nxk_ata_escribir_sector(1,0x0abcdef0u,origen+1) &&
              sim.registro[6]==0xfa && sim.registro[3]==0xf0 &&
              sim.registro[4]==0xde && sim.registro[5]==0xbc &&
              sim.escrituras==256 && sim.comandos==1 && sim.pausas[2]==4,"WRITE LBA28 alto");
    int iguales=1;
    for (unsigned i=0;i<256;++i) iguales&=sim.escritas[i]==0x7b7b;
    comprobar(iguales && origen[0]==0x7b && origen[513]==0x7b,"WRITE origen desalineado intacto");
    reiniciar();
    comprobar(nxk_ata_escribir_sector(0,0,origen)==-1 && !sim.io,"Maestro prohibido sin I/O");
    comprobar(nxk_ata_escribir_sector(2,0,origen)==-1 &&
              nxk_ata_escribir_sector(1,0x10000000,origen)==-1 &&
              nxk_ata_escribir_sector(1,UINT32_MAX,origen)==-1 &&
              nxk_ata_escribir_sector(1,0,0)==-1 && !sim.io,"WRITE limites sin I/O");
    memset(destino,0xa5,sizeof(destino));
    comprobar(nxk_ata_leer_sector(2,0,destino)==-1 &&
              nxk_ata_leer_sector(0,0x10000000,destino)==-1 &&
              nxk_ata_leer_sector(0,0,0)==-1 && !sim.io &&
              centinela(destino,sizeof(destino)),"READ limites sin efectos");
    reiniciar();
    secuencia(0,0x80,0x80,0x50); secuencia(1,0x80,0x50,0x58);
    secuencia(2,0x80,0x58,0x50);
    comprobar(!nxk_ata_leer_sector(1,0x0abcdef0u,destino+1) && sim.lecturas==256 &&
              sim.consultas[2]==3 && sim.pausas[2]==4 && destino[0]==0xa5 && destino[513]==0xa5 &&
              destino[1]==0 && destino[2]==0x12 && destino[511]==0xff &&
              destino[512]==0x12 && sim.registro[6]==0xfa,"READ transiciones y limites del buffer");
    /* Cada fallo debe conservar TODO el destino, incluso al cerrar el sector. */
    const uint8_t estados[]={0x80,0x58,0x51,0x70,0x00,0xff};
    const int errores[]={-4,-4,-3,-3,-2,-2};
    /* Disco no ocupado que nunca solicita datos: tampoco permite transferir. */
    reiniciar(); memset(destino,0xa5,sizeof(destino)); secuencia(1,0x50,0x50,0x50);
    comprobar(nxk_ata_leer_sector(0,7,destino+1)==-4 && !sim.lecturas &&
              sim.consultas[1]==1000000 && centinela(destino,sizeof(destino)),
              "READ sin DRQ agota espera sin copiar");
    reiniciar(); secuencia(1,0x50,0x50,0x50);
    comprobar(nxk_ata_escribir_sector(1,7,origen+1)==-4 && !sim.escrituras &&
              sim.consultas[1]==1000000,"WRITE sin DRQ no envia datos");
    /* Mientras BSY=1 no se interpretan ERR/DF/DRQ como estado final. */
    reiniciar(); secuencia(0,0xa1,0x88,0x50); secuencia(1,0xa1,0x88,0x58);
    secuencia(2,0xa1,0x88,0x50);
    comprobar(!nxk_ata_leer_sector(0,7,destino+1) && sim.consultas[0]==3 &&
              sim.consultas[1]==3 && sim.consultas[2]==3,"BSY precede a otros bits");
    for (unsigned fase=0;fase<3;++fase) for (unsigned e=0;e<6;++e) {
        if (fase==1 && e==1) continue; /* DRQ es exito durante transferencia. */
        reiniciar(); memset(destino,0xa5,sizeof(destino));
        secuencia(fase,estados[e],estados[e],estados[e]);
        int r=nxk_ata_leer_sector(0,7,destino+1);
        char nombre[80]; snprintf(nombre,sizeof(nombre),"READ fase%u estado%02x",fase,estados[e]);
        comprobar(r==errores[e] && centinela(destino,sizeof(destino)) &&
                  (fase!=0 || !sim.comandos) && (fase==2 || !sim.lecturas) &&
                  (errores[e]!=-4 || sim.consultas[fase]==1000000),nombre);
    }
    /* WRITE puede fallar antes, durante o despues del envio. Un error final
     * no promete deshacer lo recibido por el disco; solo informa el fallo. */
    for (unsigned fase=0;fase<3;++fase) for (unsigned e=0;e<6;++e) {
        if (fase==1 && e==1) continue;
        reiniciar(); secuencia(fase,estados[e],estados[e],estados[e]);
        int r=nxk_ata_escribir_sector(1,7,origen+1);
        char nombre[80]; snprintf(nombre,sizeof(nombre),"WRITE fase%u estado%02x",fase,estados[e]);
        comprobar(r==errores[e] && (fase!=0 || !sim.comandos) &&
                  sim.escrituras==(fase==2 ? 256u : 0u) &&
                  (errores[e]!=-4 || sim.consultas[fase]==1000000),nombre);
    }
    for (unsigned e=0;e<6;++e) {
        reiniciar(); memset(&info,0xa5,sizeof(info));
        secuencia(2,estados[e],estados[e],estados[e]);
        int r=nxk_ata_identificar(1,&info);
        char nombre[80]; snprintf(nombre,sizeof(nombre),"IDENTIFY final estado%02x",estados[e]);
        comprobar(r==errores[e] && centinela(&info,sizeof(info)),nombre);
        unsigned antes=sim.io;
        comprobar(nxk_ata_leer_sector(1,0,destino)==-1 && sim.io==antes,
                  "IDENTIFY fallido no publica capacidad");
    }
    reiniciar(); comprobar(!nxk_ata_identificar(1,&info),"Recuperar IDENTIFY");
    reiniciar(); comprobar(!nxk_ata_sincronizar(1) && sim.comando==0xe7 &&
                          !sim.escrituras,"FLUSH sin datos");
    for (unsigned e=0;e<6;++e) {
        reiniciar(); secuencia(2,estados[e],estados[e],estados[e]);
        comprobar(nxk_ata_sincronizar(1)==errores[e],"FLUSH rechazo/timeout");
    }
    reiniciar(); comprobar(nxk_ata_sincronizar(0)==-1 && !sim.io,"FLUSH maestro prohibido");
    printf("ATA HOST: %u comprobaciones, %u fallos\n",casos,fallos);
    return fallos ? 1 : 0;
}
