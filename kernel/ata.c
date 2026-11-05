/* Controlador original ATA PIO de lectura y escritura acotada. Puente C para x86 BIOS/QEMU.
 * Un comando por vez, canal primario, unidades 0/1, sectores de 512 bytes.
 * Escritura solo en esclavo; los sondeos tienen limite, sin IRQ ATA ni DMA. */
#include "ata.h"
#if defined(__i386__)
static uint32_t capacidad[2];
static uint8_t entrada(uint16_t p) { uint8_t v; __asm__ volatile("inb %1,%0":"=a"(v):"Nd"(p)); return v; }
static void salida(uint16_t p,uint8_t v) { __asm__ volatile("outb %0,%1" :: "a"(v),"Nd"(p)); }
static void pausa(void) { for (unsigned i=0;i<4;++i) (void)entrada(0x3f6); }
static int esperar(int datos) {
    for (unsigned i=0;i<1000000;++i) {
        uint8_t s=entrada(0x1f7);
        if (!s || s==0xff) return -2;
        if (s&0x80) continue;
        if (s&0x21) return -3;
        if (!datos || (s&8)) return 0;
    }
    return -4;
}
/* Un comando de escritura/flush solo termina con BSY y DRQ despejados.
 * No interpretar una solicitud de datos pendiente como operacion completada. */
static int esperar_final(void) {
    for (unsigned i=0;i<1000000;++i) {
        uint8_t s=entrada(0x1f7);
        if (!s || s==0xff) return -2;
        if (s&0x80) continue;
        if (s&0x21) return -3;
        if (!(s&8)) return 0;
    }
    return -4;
}
static void leer_palabras(uint16_t *p) {
    for (unsigned i=0;i<256;++i) __asm__ volatile("inw %1,%0":"=a"(p[i]):"Nd"((uint16_t)0x1f0));
}
int nxk_ata_identificar(unsigned unidad,struct nxk_ata_info *info) {
    if (unidad>1 || !info) return -1;
    capacidad[unidad]=0;
    salida(0x3f6,2); salida(0x1f6,(uint8_t)(0xa0|(unidad<<4))); pausa();
    /* IDENTIFY tambien distingue unidades ATAPI, que no soportamos. */
    salida(0x1f2,0); salida(0x1f3,0); salida(0x1f4,0); salida(0x1f5,0);
    salida(0x1f7,0xec); pausa();
    int r=esperar(1); if (r) return r;
    if (entrada(0x1f4) || entrada(0x1f5)) return -5;
    uint16_t datos[256]; leer_palabras(datos);
    if (esperar(0)) return -3;
    if (!(datos[49]&0x200)) return -5;
    uint32_t n=datos[60]|(uint32_t)datos[61]<<16;
    if (!n) return -5;
    if (n>0x10000000u) n=0x10000000u;
    struct nxk_ata_info temporal={0}; temporal.sectores=n;
    for (unsigned i=0;i<20;++i) {
        temporal.modelo[i*2]=(char)(datos[27+i]>>8);
        temporal.modelo[i*2+1]=(char)datos[27+i];
    }
    for (unsigned i=0;i<40;++i) if ((unsigned char)temporal.modelo[i]<32 || (unsigned char)temporal.modelo[i]>126) temporal.modelo[i]='?';
    temporal.modelo[40]=0;
    for (int i=39;i>=0 && temporal.modelo[i]==' ';--i) temporal.modelo[i]=0;
    *info=temporal; capacidad[unidad]=n; return 0;
}
int nxk_ata_leer_sector(unsigned unidad,uint32_t lba,void *destino) {
    if (unidad>1 || !destino || !capacidad[unidad] || lba>=capacidad[unidad] || lba>=0x10000000u) return -1;
    salida(0x1f6,(uint8_t)(0xe0|(unidad<<4)|(lba>>24))); pausa();
    int r=esperar(0); if (r) return r;
    salida(0x1f2,1); salida(0x1f3,(uint8_t)lba);
    salida(0x1f4,(uint8_t)(lba>>8)); salida(0x1f5,(uint8_t)(lba>>16));
    salida(0x1f7,0x20); pausa();
    r=esperar(1); if (r) return r;
    uint16_t datos[256]; leer_palabras(datos);
    r=esperar(0); if (r) return r;
    memcpy(destino,datos,512); return 0;
}
/* Solo se permite el esclavo: nunca sobrescribir el disco de arranque maestro.
 * El llamador valida formato y autorizacion del volumen antes de llegar aqui. */
int nxk_ata_escribir_sector(unsigned unidad,uint32_t lba,const void *origen) {
    if (unidad!=1 || !origen || !capacidad[1] || lba>=capacidad[1] || lba>=0x10000000u) return -1;
    uint16_t datos[256]; memcpy(datos,origen,512);
    salida(0x1f6,(uint8_t)(0xf0|(lba>>24))); pausa();
    int r=esperar_final(); if (r) return r;
    salida(0x1f2,1); salida(0x1f3,(uint8_t)lba);
    salida(0x1f4,(uint8_t)(lba>>8)); salida(0x1f5,(uint8_t)(lba>>16));
    salida(0x1f7,0x30); pausa(); /* WRITE SECTORS, exactamente 512 bytes. */
    r=esperar(1); if (r) return r;
    for (unsigned i=0;i<256;++i)
        __asm__ volatile("outw %0,%1" :: "a"(datos[i]),"Nd"((uint16_t)0x1f0));
    pausa(); return esperar_final();
}
int nxk_ata_sincronizar(unsigned unidad) {
    if (unidad!=1 || !capacidad[1]) return -1;
    salida(0x1f6,0xf0); pausa();
    int r=esperar_final(); if (r) return r;
    salida(0x1f7,0xe7); pausa(); /* FLUSH CACHE: persistir escrituras anteriores. */
    return esperar_final();
}
int nxk_ata_probar(void) {
    struct nxk_ata_info info;
    unsigned char sector[514]; memset(sector,0xa5,sizeof(sector));
    if (nxk_ata_leer_sector(0,0,sector)!=-1) return 1;
    /* Contratos negativos solamente: esta prueba nunca envia escrituras. */
    if (nxk_ata_escribir_sector(0,0,sector)!=-1 || nxk_ata_escribir_sector(2,0,sector)!=-1 ||
        nxk_ata_escribir_sector(1,0,0)!=-1 || nxk_ata_escribir_sector(1,0,sector)!=-1 ||
        nxk_ata_escribir_sector(1,0x10000000u,sector)!=-1 ||
        nxk_ata_sincronizar(0)!=-1 || nxk_ata_sincronizar(2)!=-1 || nxk_ata_sincronizar(1)!=-1) return 12;
    if (nxk_ata_identificar(0,&info) || info.sectores!=2048) return 2;
    if (nxk_ata_leer_sector(0,0,sector+1) || sector[0]!=0xa5 || sector[513]!=0xa5 ||
        sector[511]!=0x55 || sector[512]!=0xaa) return 3;
    if (nxk_ata_leer_sector(0,1,sector+1)) return 8;
    for (unsigned i=0;i<512;++i) if (sector[i+1]!=((const unsigned char *)0x10000)[i]) return 9;
    memset(sector,0xa5,sizeof(sector));
    if (nxk_ata_leer_sector(0,info.sectores,sector)!=-1 ||
        nxk_ata_leer_sector(2,0,sector)!=-1 || nxk_ata_leer_sector(0,0,0)!=-1) return 4;
    for (unsigned i=0;i<514;++i) if (sector[i]!=0xa5) return 5;
    if (nxk_ata_leer_sector(0,info.sectores-1,sector)) return 6;
    for (unsigned i=0;i<512;++i) if (sector[i]) return 7;
    struct nxk_ata_info ausente;
    if (!nxk_ata_identificar(1,&ausente) || nxk_ata_leer_sector(1,0,sector)!=-1) return 10;
    if (nxk_ata_leer_sector(0,0,sector) || sector[510]!=0x55 || sector[511]!=0xaa) return 11;
    return 0;
}
#else
int nxk_ata_identificar(unsigned u,struct nxk_ata_info *i) { (void)u;(void)i;return -64; }
int nxk_ata_leer_sector(unsigned u,uint32_t l,void *p) { (void)u;(void)l;(void)p;return -64; }
int nxk_ata_escribir_sector(unsigned u,uint32_t l,const void *p) { (void)u;(void)l;(void)p;return -64; }
int nxk_ata_sincronizar(unsigned u) { (void)u;return -64; }
int nxk_ata_probar(void) { return 0; }
#endif
