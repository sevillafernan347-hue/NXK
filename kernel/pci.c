/* Enumeracion PCI propia mediante CF8/CFC. Firmware configura puentes/buses.
 * Recorre todos los numeros de bus; no reasigna recursos ni dimensiona BARs.
 * Puente C, pendiente estructuras y E/S de NEXTRY. */
#include "pci.h"
#if defined(__i386__)
static struct nxk_pci_dispositivo dispositivos[NXK_PCI_MAX];
static struct nxk_pci_resumen resumen;
static uint32_t entrada32(uint16_t p) { uint32_t v; __asm__ volatile("inl %1,%0":"=a"(v):"Nd"(p)); return v; }
static void salida32(uint16_t p,uint32_t v) { __asm__ volatile("outl %0,%1"::"a"(v),"Nd"(p)); }
/* Conservar selector previo y estado IF. IRQ no puede separar selector/dato. */
static uint32_t leer(unsigned b,unsigned d,unsigned f,unsigned r) {
    uint32_t flags,anterior,valor;
    __asm__ volatile("pushfl; popl %0; cli":"=r"(flags)::"memory");
    anterior=entrada32(0xcf8);
    salida32(0xcf8,0x80000000u|(b<<16)|(d<<11)|(f<<8)|r);
    valor=entrada32(0xcfc); salida32(0xcf8,anterior);
    __asm__ volatile("pushl %0; popfl"::"r"(flags):"memory","cc");
    return valor;
}
int nxk_pci_leer32(unsigned b,unsigned d,unsigned f,unsigned r,uint32_t *s) {
    if (!s || b>255 || d>31 || f>7 || r>252 || (r&3)) return -1;
    *s=leer(b,d,f,r); return 0;
}
typedef uint32_t (*lector)(unsigned,unsigned,unsigned,unsigned);
static void explorar(lector obtener) {
    memset(dispositivos,0,sizeof(dispositivos)); memset(&resumen,0,sizeof(resumen));
    for (unsigned b=0;b<256;++b) for (unsigned d=0;d<32;++d) {
        uint32_t id=obtener(b,d,0,0);
        if ((id&0xffffu)==0xffffu || !(id&0xffffu)) continue;
        unsigned funciones=(obtener(b,d,0,12)&0x00800000u) ? 8 : 1;
        for (unsigned f=0;f<funciones;++f) {
            if (f) id=obtener(b,d,f,0);
            if ((id&0xffffu)==0xffffu || !(id&0xffffu)) continue;
            ++resumen.detectados;
            if (resumen.guardados==NXK_PCI_MAX) { ++resumen.omitidos; continue; }
            uint32_t clase=obtener(b,d,f,8),cabecera=obtener(b,d,f,12);
            struct nxk_pci_dispositivo *p=&dispositivos[resumen.guardados++];
            p->bus=b; p->ranura=d; p->funcion=f; p->fabricante=id&0xffff; p->dispositivo=id>>16;
            p->clase=clase>>24; p->subclase=(clase>>16)&255; p->interfaz=(clase>>8)&255;
            p->revision=clase&255; p->cabecera=(cabecera>>16)&255;
            p->irq=obtener(b,d,f,60)&255;
        }
    }
}
int nxk_pci_explorar(void) { explorar(leer); return 0; }
int nxk_pci_resumen(struct nxk_pci_resumen *s) { if (!s) return -1; *s=resumen; return 0; }
int nxk_pci_consultar(unsigned i,struct nxk_pci_dispositivo *s) {
    if (!s) return -1;
    if (i>=resumen.guardados) return -2;
    *s=dispositivos[i]; return 0;
}
#ifdef NXK_TEST
static unsigned cantidad_falsa;
static uint32_t falso(unsigned b,unsigned d,unsigned f,unsigned r) {
    if (b!=1 || d*8+f>=cantidad_falsa) return UINT32_MAX;
    if (!r) return 0x56781234;
    if (r==8) return 0x02000103;
    if (r==12) return 0x00800000; /* Multifuncion incluso con huecos al final. */
    return 11;
}
int nxk_pci_probar(void) {
    size_t heap=nxk_heap_used(); uint32_t valor=0xdeadbeef;
    if (nxk_pci_leer32(256,0,0,0,&valor)!=-1 || nxk_pci_leer32(0,32,0,0,&valor)!=-1 ||
        nxk_pci_leer32(0,0,8,0,&valor)!=-1 || nxk_pci_leer32(0,0,0,256,&valor)!=-1 ||
        nxk_pci_leer32(0,0,0,1,&valor)!=-1 || nxk_pci_leer32(0,0,0,0,0)!=-1 || valor!=0xdeadbeef) return 1;
    cantidad_falsa=70; explorar(falso);
    struct nxk_pci_resumen r; struct nxk_pci_dispositivo p;
    if (nxk_pci_resumen(&r) || r.detectados!=70 || r.guardados!=64 || r.omitidos!=6 ||
        nxk_pci_consultar(63,&p) || p.bus!=1 || p.ranura!=7 || p.funcion!=7 ||
        p.fabricante!=0x1234 || p.dispositivo!=0x5678 || p.clase!=2 || p.interfaz!=1 || p.irq!=11) return 2;
    memset(&p,0xa5,sizeof(p));
    if (nxk_pci_consultar(64,&p)!=-2 || nxk_pci_consultar(0,0)!=-1 || nxk_pci_resumen(0)!=-1) return 3;
    for (unsigned i=0;i<sizeof(p);++i) if (((unsigned char *)&p)[i]!=0xa5) return 4;
    cantidad_falsa=1; explorar(falso);
    if (resumen.detectados!=1 || resumen.guardados!=1 || resumen.omitidos) return 5;
    uint32_t selector=entrada32(0xcf8),antes,despues;
    __asm__ volatile("pushfl; popl %0":"=r"(antes));
    if (nxk_pci_explorar() || !resumen.detectados || entrada32(0xcf8)!=selector) return 6;
    __asm__ volatile("pushfl; popl %0":"=r"(despues));
    if ((antes^despues)&0x200u || nxk_heap_used()!=heap) return 7;
    return 0;
}
#endif
#else
int nxk_pci_explorar(void) { return -64; }
int nxk_pci_resumen(struct nxk_pci_resumen *s) { (void)s;return -64; }
int nxk_pci_consultar(unsigned i,struct nxk_pci_dispositivo *s) { (void)i;(void)s;return -64; }
int nxk_pci_leer32(unsigned b,unsigned d,unsigned f,unsigned r,uint32_t *s) { (void)b;(void)d;(void)f;(void)r;(void)s;return -64; }
#endif
