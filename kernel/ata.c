/* Controlador original ATA PIO de lectura y escritura acotada. Puente C para x86 BIOS/QEMU.
 * Un comando por vez, canal primario, unidades 0/1, sectores de 512 bytes.
 * Escritura solo en esclavo; los sondeos tienen limite, sin IRQ ATA ni DMA. */
#include "ata.h"
#if defined(__i386__) || defined(NXK_ATA_PRUEBA_HOST)
static uint32_t capacidad[2];
static uint32_t revision_esclavo;
static uint32_t generacion_canal;
static int reinicio_pendiente;
static struct nxk_ata_diagnostico diagnosticos[2];
uint32_t nxk_ata_revision_esclavo(void) { return revision_esclavo; }
uint32_t nxk_ata_generacion_canal(void) { return generacion_canal; }
static void cambiar_revision_esclavo(void) {
    /* Saturar evita que una revision vieja vuelva a coincidir por overflow. */
    if (revision_esclavo!=UINT32_MAX) ++revision_esclavo;
}
int nxk_ata_consultar(unsigned unidad,struct nxk_ata_diagnostico *salida) {
    if (unidad>1 || !salida) return -1;
    *salida=diagnosticos[unidad]; return 0;
}
static struct nxk_ata_diagnostico *iniciar(unsigned unidad,uint32_t comando,uint32_t lba) {
    struct nxk_ata_diagnostico *d=&diagnosticos[unidad];
    memset(d,0,sizeof(*d)); d->comando=comando; d->lba=lba; return d;
}
static int terminar(struct nxk_ata_diagnostico *d,int resultado) {
    d->resultado=resultado; return resultado;
}
#if defined(NXK_ATA_PRUEBA_HOST)
/* Solo el ejecutable de pruebas define este backend; no accede a puertos reales. */
static uint8_t entrada(uint16_t p);
static void salida(uint16_t p,uint8_t v);
static uint16_t entrada_palabra(void);
static void salida_palabra(uint16_t v);
static uint32_t nxk_reloj_ticks(void);
#else
extern uint32_t nxk_reloj_ticks(void);
static uint8_t entrada(uint16_t p) { uint8_t v; __asm__ volatile("inb %1,%0":"=a"(v):"Nd"(p)); return v; }
static void salida(uint16_t p,uint8_t v) { __asm__ volatile("outb %0,%1" :: "a"(v),"Nd"(p)); }
static uint16_t entrada_palabra(void) { uint16_t v; __asm__ volatile("inw %1,%0":"=a"(v):"Nd"((uint16_t)0x1f0)); return v; }
static void salida_palabra(uint16_t v) { __asm__ volatile("outw %0,%1" :: "a"(v),"Nd"((uint16_t)0x1f0)); }
#endif
static void pausa(void) { for (unsigned i=0;i<4;++i) (void)entrada(0x3f6); }
/* Dos avances dejan al menos un periodo entero, aun si el primero llega
 * justo al iniciar la espera. El tope protege si no se reciben IRQ del PIT. */
static int esperar_ticks(uint32_t inicio,uint32_t ticks) {
    for (unsigned i=0;i<8000000u;++i) {
        if (nxk_reloj_ticks()-inicio>=ticks) return 0;
#if !defined(NXK_ATA_PRUEBA_HOST)
        __asm__ volatile("pause");
#endif
    }
    return -6;
}

int nxk_ata_reiniciar(struct nxk_ata_reinicio *resultado) {
    if (!resultado) return -1;
    *resultado=(struct nxk_ata_reinicio){0};
    int r=esperar_ticks(nxk_reloj_ticks(),2);
    if (r) return r;

    /* SRST afecta a ambos dispositivos. Las capacidades no sobreviven al
     * pulso. nIEN queda activo para mantener deshabilitada la IRQ de ATA. */
    capacidad[0]=0;
    capacidad[1]=0;
    cambiar_revision_esclavo();
    if (generacion_canal!=UINT32_MAX) ++generacion_canal;
    reinicio_pendiente=1;
    salida(0x3f6,0x06);
    resultado->emitido=1;
    r=esperar_ticks(nxk_reloj_ticks(),2);
    salida(0x3f6,0x02); /* Liberar SRST incluso si el reloj dejo de avanzar. */
    if (r) return r;
    r=esperar_ticks(nxk_reloj_ticks(),2);
    if (r) return r;

    /* Un controlador puede retener la seleccion previa, incluso de una
     * unidad ausente. Seleccionar el maestro cuando cese la espera y
     * comprobar su estado antes de leer diagnostico y firma. */
    uint32_t inicio=nxk_reloj_ticks();
    int maestro_seleccionado=0;
    for (;;) {
        uint8_t estado=entrada(0x3f6);
        resultado->estado=estado;
        int ausente=!estado || estado==0xff;
        if (ausente || !(estado&0x88)) {
            if (maestro_seleccionado) {
                if (ausente) return -2;
                break;
            }
            salida(0x1f6,0xa0);
            maestro_seleccionado=1;
            pausa();
            continue;
        }
        uint32_t ahora=nxk_reloj_ticks();
        if (ahora-inicio>=3200u) return -4;
        r=esperar_ticks(ahora,1);
        if (r) return r;
    }

    resultado->diagnostico=entrada(0x1f1);
    resultado->firma=entrada(0x1f2);
    resultado->firma|=(uint32_t)entrada(0x1f3)<<8;
    resultado->firma|=(uint32_t)entrada(0x1f4)<<16;
    resultado->firma|=(uint32_t)entrada(0x1f5)<<24;
    resultado->firma_valida=1;
    /* El bit 7 del diagnostico indica fallo del esclavo. El maestro puede
     * seguir operativo; la capacidad del esclavo requiere otro IDENTIFY. */
    if ((resultado->estado&0x20) || (resultado->diagnostico&0x7f)!=1 ||
        resultado->firma!=0x00000101u) return -3;
    reinicio_pendiente=0;
    return 0;
}

/* Misma politica para todas las fases. El diagnostico guarda el ultimo sondeo;
 * no releer Status al consultar, pues podria ocultar la causa original. */
static int esperar(struct nxk_ata_diagnostico *d,unsigned fase,int datos) {
    d->fase=fase;
    /* PIT a unos 100 Hz. FLUSH puede tardar mas que una transferencia. */
    const uint32_t plazo=d->comando==0xe7 && fase==3 ? 1000 : 200;
    const uint32_t inicio=nxk_reloj_ticks();
    for (unsigned i=0;i<1000000;++i) {
        uint8_t s=entrada(0x1f7);
        d->estado=s; ++d->sondeos;
        if (!s || s==0xff) return -2;
        if (!(s&0x80)) {
            /* En fase previa se acepta ERR residual, pero DF sigue siendo fallo. */
            if ((s&0x20) || (fase!=1 && (s&1))) {
                /* Error requiere BSY=DRQ=0 y ERR=1. */
                if ((s&9)==1) { d->error=entrada(0x1f1); d->error_valido=1; }
                return -3;
            }
            /* Esperar DRQ en datos y su desactivacion en previa y cierre. */
            if (datos ? (s&8)!=0 : (s&8)==0) return 0;
        }
        /* La resta modular admite el reinicio del contador al desbordar.
         * El limite de sondeos tambien funciona si no llegan ticks. */
        if ((i&31u)==31u && nxk_reloj_ticks()-inicio>=plazo) {
            d->limite=NXK_ATA_LIMITE_PIT;
            return -4;
        }
    }
    d->limite=NXK_ATA_LIMITE_SONDEOS;
    return -4;
}
static void leer_palabras(uint16_t *p) {
    for (unsigned i=0;i<256;++i) p[i]=entrada_palabra();
}
int nxk_ata_identificar(unsigned unidad,struct nxk_ata_info *info) {
    if (unidad>1 || !info) return -1;
    if (reinicio_pendiente) return -6;
    if (unidad==1) cambiar_revision_esclavo();
    struct nxk_ata_diagnostico *d=iniciar(unidad,0xec,0);
    capacidad[unidad]=0;
    salida(0x3f6,2); salida(0x1f6,(uint8_t)(0xa0|(unidad<<4))); pausa();
    int r=esperar(d,1,0);
    if (r) return terminar(d,r);
    /* IDENTIFY tambien distingue unidades ATAPI, que no soportamos. */
    salida(0x1f2,0); salida(0x1f3,0); salida(0x1f4,0); salida(0x1f5,0);
    salida(0x1f7,0xec); d->emitido=1; pausa();
    r=esperar(d,2,1); if (r) return terminar(d,r);
    if (entrada(0x1f4) || entrada(0x1f5)) return terminar(d,-5);
    uint16_t datos[256]; leer_palabras(datos);
    /* La ultima palabra no basta: esperar el cierre antes de publicar capacidad. */
    pausa(); r=esperar(d,3,0); if (r) return terminar(d,r);
    if (!(datos[49]&0x200)) return terminar(d,-5);
    uint32_t n=datos[60]|(uint32_t)datos[61]<<16;
    if (!n) return terminar(d,-5);
    if (n>0x10000000u) n=0x10000000u;
    struct nxk_ata_info temporal={0}; temporal.sectores=n;
    for (unsigned i=0;i<20;++i) {
        temporal.modelo[i*2]=(char)(datos[27+i]>>8);
        temporal.modelo[i*2+1]=(char)datos[27+i];
    }
    for (unsigned i=0;i<40;++i) if ((unsigned char)temporal.modelo[i]<32 || (unsigned char)temporal.modelo[i]>126) temporal.modelo[i]='?';
    temporal.modelo[40]=0;
    for (int i=39;i>=0 && temporal.modelo[i]==' ';--i) temporal.modelo[i]=0;
    *info=temporal; capacidad[unidad]=n; return terminar(d,0);
}
int nxk_ata_leer_sector(unsigned unidad,uint32_t lba,void *destino) {
    if (unidad>1 || !destino || !capacidad[unidad] || lba>=capacidad[unidad] || lba>=0x10000000u) return -1;
    struct nxk_ata_diagnostico *d=iniciar(unidad,0x20,lba);
    salida(0x1f6,(uint8_t)(0xe0|(unidad<<4)|(lba>>24))); pausa();
    int r=esperar(d,1,0); if (r) return terminar(d,r);
    salida(0x1f2,1); salida(0x1f3,(uint8_t)lba);
    salida(0x1f4,(uint8_t)(lba>>8)); salida(0x1f5,(uint8_t)(lba>>16));
    salida(0x1f7,0x20); d->emitido=1; pausa();
    r=esperar(d,2,1); if (r) return terminar(d,r);
    uint16_t datos[256]; leer_palabras(datos);
    /* Conservar destino intacto hasta completar tambien la fase final del disco. */
    pausa(); r=esperar(d,3,0); if (r) return terminar(d,r);
    memcpy(destino,datos,512); return terminar(d,0);
}
/* Solo se permite el esclavo: nunca sobrescribir el disco de arranque maestro.
 * El llamador valida formato y autorizacion del volumen antes de llegar aqui. */
int nxk_ata_escribir_sector(unsigned unidad,uint32_t lba,const void *origen) {
    if (unidad!=1 || !origen || !capacidad[1] || lba>=capacidad[1] || lba>=0x10000000u) return -1;
    cambiar_revision_esclavo();
    struct nxk_ata_diagnostico *d=iniciar(unidad,0x30,lba);
    uint16_t datos[256]; memcpy(datos,origen,512);
    salida(0x1f6,(uint8_t)(0xf0|(lba>>24))); pausa();
    int r=esperar(d,1,0); if (r) return terminar(d,r);
    salida(0x1f2,1); salida(0x1f3,(uint8_t)lba);
    salida(0x1f4,(uint8_t)(lba>>8)); salida(0x1f5,(uint8_t)(lba>>16));
    salida(0x1f7,0x30); d->emitido=1; pausa(); /* WRITE SECTORS, exactamente 512 bytes. */
    r=esperar(d,2,1); if (r) return terminar(d,r);
    for (unsigned i=0;i<256;++i)
        salida_palabra(datos[i]);
    pausa(); return terminar(d,esperar(d,3,0));
}
int nxk_ata_sincronizar(unsigned unidad) {
    if (unidad!=1 || !capacidad[1]) return -1;
    struct nxk_ata_diagnostico *d=iniciar(unidad,0xe7,0);
    salida(0x1f6,0xf0); pausa();
    int r=esperar(d,1,0); if (r) return terminar(d,r);
    salida(0x1f7,0xe7); d->emitido=1; pausa(); /* FLUSH CACHE: persistir escrituras anteriores. */
    return terminar(d,esperar(d,3,0));
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
    struct nxk_ata_diagnostico d;
    if (nxk_ata_consultar(0,&d) || d.comando!=0x20 || d.lba || d.fase!=3 ||
        !d.emitido || d.resultado || d.error_valido || !d.sondeos || d.limite) return 13;
    return 0;
}
#else
uint32_t nxk_ata_revision_esclavo(void) { return UINT32_MAX; }
uint32_t nxk_ata_generacion_canal(void) { return 0; }
int nxk_ata_reiniciar(struct nxk_ata_reinicio *r) { (void)r;return -64; }
int nxk_ata_consultar(unsigned u,struct nxk_ata_diagnostico *s) { (void)u;(void)s;return -64; }
int nxk_ata_identificar(unsigned u,struct nxk_ata_info *i) { (void)u;(void)i;return -64; }
int nxk_ata_leer_sector(unsigned u,uint32_t l,void *p) { (void)u;(void)l;(void)p;return -64; }
int nxk_ata_escribir_sector(unsigned u,uint32_t l,const void *p) { (void)u;(void)l;(void)p;return -64; }
int nxk_ata_sincronizar(unsigned u) { (void)u;return -64; }
int nxk_ata_probar(void) {
    struct nxk_ata_diagnostico d; memset(&d,0xa5,sizeof(d));
    if (nxk_ata_consultar(0,&d)!=-64) return 1;
    for (unsigned i=0;i<sizeof(d);++i) if (((unsigned char *)&d)[i]!=0xa5) return 2;
    return 0;
}
#endif
