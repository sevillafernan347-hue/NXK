/* GUIA DE LECTURA NXK - compatibilidad/windows/pe.c
 * Valida PE32, carga secciones y conecta tres funciones Win32. No ejecuta apps Windows generales.
 * Contexto y orden de estudio: ver LEER-CODIGO.md en la carpeta principal.
 */
/* Puente experimental PE32 / Win32. Solo codigo propio y confiable, en ring 0. */
#include "pe.h"
#include "nxk.h"
#if defined(__i386__)
#define BASE 0x400000u
#define LIMITE (128u*1024u)
extern const unsigned char pe_demo_inicio[], pe_demo_fin[];
extern void nxk_pe_salto(void (*entrada)(void));
extern _Noreturn void nxk_pe_regresar(void);
uint32_t nxk_pe_pila;
static uint32_t imagen_longitud;
static volatile uint32_t codigo_salida;
static unsigned char *const imagen=(unsigned char *)(uintptr_t)BASE;
static uint16_t u16(const unsigned char *p) { return p[0] | (uint16_t)p[1]<<8; }
static uint32_t u32(const unsigned char *p) { return p[0] | (uint32_t)p[1]<<8 | (uint32_t)p[2]<<16 | (uint32_t)p[3]<<24; }
static int rango(uint32_t inicio, uint32_t cantidad, size_t total) {
    /* Restar despues de validar inicio evita desbordar inicio + cantidad. */
    return inicio<=total && cantidad<=total-inicio;
}
static int puntero_valido(uintptr_t p, uint32_t n) {
    return (p>=BASE && rango(p-BASE,n,imagen_longitud)) ||
           (p>=0x80000 && rango(p-0x80000,n,0x10000));
}
static void *__attribute__((stdcall)) obtener_salida(uint32_t tipo) {
    if (tipo==(uint32_t)-11) return (void *)1;
    if (tipo==(uint32_t)-12) return (void *)2;
    return (void *)(uintptr_t)0xffffffff;
}
static int __attribute__((stdcall)) escribir_archivo(void *h, const void *datos,
        uint32_t cantidad, uint32_t *escritos, void *asincrono) {
    if (!escritos || ((uintptr_t)escritos & 3) || !puntero_valido((uintptr_t)escritos,4)) return 0;
    *escritos=0;
    if ((h!=(void *)1 && h!=(void *)2) || asincrono || cantidad>4096 ||
            !puntero_valido((uintptr_t)datos,cantidad)) return 0;
    nxk_pe_salida(datos,cantidad);
    *escritos=cantidad;
    return 1;
}
static _Noreturn void __attribute__((stdcall)) terminar(uint32_t codigo) {
    codigo_salida=codigo;
    nxk_pe_regresar();
}
static int nombre(uint32_t rva, const char *esperado, int ignorar_mayusculas) {
    for (unsigned i=0; i<64; ++i) {
        if (!rango(rva,i+1,imagen_longitud)) return 0;
        unsigned char a=imagen[rva+i], b=(unsigned char)esperado[i];
        if (ignorar_mayusculas) {
            if (a>='a' && a<='z') a-=32;
            if (b>='a' && b<='z') b-=32;
        }
        if (a!=b) return 0;
        if (!a) return 1;
    }
    return 0;
}
static uintptr_t resolver(uint32_t rva) {
    /* Lista cerrada de las tres funciones Win32 disponibles en este prototipo. */
    if (nombre(rva,"GetStdHandle",0)) return (uintptr_t)obtener_salida;
    if (nombre(rva,"WriteFile",0)) return (uintptr_t)escribir_archivo;
    if (nombre(rva,"ExitProcess",0)) return (uintptr_t)terminar;
    return 0;
}
int nxk_pe_cargar(const unsigned char *p, size_t longitud) {
    if (longitud<64 || p[0]!='M' || p[1]!='Z') return -1;
    uint32_t pe=u32(p+60);
    if (!rango(pe,24,longitud) || u32(p+pe)!=0x4550 || u16(p+pe+4)!=0x14c) return -2;
    uint32_t secciones=u16(p+pe+6), opt=pe+24, tamano_opt=u16(p+pe+20);
    uint16_t atributos=u16(p+pe+22);
    if (!secciones || secciones>8 || tamano_opt<224 || !rango(opt,tamano_opt,longitud) ||
            !(atributos&2) || (atributos&0x2000) || u16(p+opt)!=0x10b) return -3;
    uint32_t entrada=u32(p+opt+16), base=u32(p+opt+28), total=u32(p+opt+56), cabecera=u32(p+opt+60);
    if (base!=BASE || !total || total>LIMITE || entrada>=total || !cabecera || cabecera>total || cabecera>longitud) return -4;
    if (u16(p+opt+68)!=3 || u32(p+opt+92)<16) return -5;
    /* TLS, imports diferidos y CLR requieren infraestructura aun no implementada. */
    if (u32(p+opt+168) || u32(p+opt+172) || u32(p+opt+200) || u32(p+opt+204) ||
            u32(p+opt+208) || u32(p+opt+212)) return -6;
    uint32_t tabla=opt+tamano_opt;
    if (!rango(tabla,secciones*40,longitud) || tabla+secciones*40>cabecera) return -7;
    uint32_t inicios[8], finales[8]; int entrada_ejecutable=0;
    for (uint32_t i=0; i<secciones; ++i) {
        const unsigned char *s=p+tabla+i*40;
        uint32_t virtual=u32(s+8), va=u32(s+12), crudo=u32(s+16), pos=u32(s+20);
        uint32_t ocupa=virtual>crudo ? virtual : crudo;
        if (va<cabecera || !rango(va,ocupa,total) || !rango(pos,crudo,longitud)) return -8;
        inicios[i]=va; finales[i]=va+ocupa;
        for (uint32_t j=0; j<i; ++j) if (va<finales[j] && inicios[j]<finales[i]) return -8;
        if (entrada>=va && entrada-va<ocupa && (u32(s+36)&0x20000000)) entrada_ejecutable=1;
    }
    if (!entrada_ejecutable) return -9;
    memset(imagen,0,total); imagen_longitud=total;
    /* Copias de cabecera y secciones mediante NEXTRY ASM en 32 bits.
       Las comprobaciones anteriores verificaron los rangos del archivo. */
    memcpy(imagen,p,cabecera);
    for (uint32_t i=0; i<secciones; ++i) {
        const unsigned char *s=p+tabla+i*40;
        memcpy(imagen+u32(s+12),p+u32(s+20),u32(s+16));
    }
    uint32_t imports=u32(p+opt+104), bytes=u32(p+opt+108);
    if (!imports || bytes<20 || !rango(imports,bytes,total)) return -10;
    unsigned terminado=0;
    for (uint32_t d=0; d<16 && rango(d*20,20,bytes); ++d) {
        const unsigned char *desc=imagen+imports+d*20;
        uint32_t nombres=u32(desc), dll=u32(desc+12), destinos=u32(desc+16);
        if (!nombres && !dll && !destinos && !u32(desc+4) && !u32(desc+8)) { terminado=1; break; }
        if (!nombre(dll,"KERNEL32.dll",1) || !destinos) return -11;
        if (!nombres) nombres=destinos;
        unsigned fin_tabla=0;
        for (uint32_t t=0; t<128; ++t) {
            if (!rango(nombres,4*(t+1),total) || !rango(destinos,4*(t+1),total)) return -12;
            uint32_t valor=u32(imagen+nombres+4*t);
            if (!valor) { fin_tabla=1; break; }
            if ((valor&0x80000000) || !rango(valor,3,total)) return -12;
            uintptr_t f=resolver(valor+2);
            if (!f) return -13;
            uint32_t pos=destinos+4*t;
            for (unsigned b=0; b<4; ++b) imagen[pos+b]=(unsigned char)(f>>(8*b));
        }
        if (!fin_tabla) return -12;
    }
    if (!terminado) return -14;
    codigo_salida=0;
    nxk_pe_salto((void (*)(void))(uintptr_t)(BASE+entrada));
    return codigo_salida>0x7fffffff ? -15 : (int)codigo_salida;
}
int nxk_pe_demo(void) {
    return nxk_pe_cargar(pe_demo_inicio,(uintptr_t)pe_demo_fin-(uintptr_t)pe_demo_inicio);
}
#else
int nxk_pe_cargar(const unsigned char *p, size_t longitud) { (void)p; (void)longitud; return -64; }
int nxk_pe_demo(void) { return -64; }
#endif
