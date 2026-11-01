/* GUIA DE LECTURA NXK - kernel/nxe.c
 * Valida y ejecuta nuestro formato NXE en una direccion fija. Las apps comparten privilegios con el kernel.
 * Contexto y orden de estudio: ver LEER-CODIGO.md en la carpeta principal.
 */
#include "nxe.h"
#include "nxk.h"
extern const unsigned char nxe_demo_inicio[], nxe_demo_fin[];
static uint32_t u32(const unsigned char *p) {
    return (uint32_t)p[0] | (uint32_t)p[1]<<8 | (uint32_t)p[2]<<16 | (uint32_t)p[3]<<24;
}
static const NXE_API api = {
    1, sizeof(NXE_API), nxk_es_mostrar, nxk_es_numero,
    nxk_es_memoria_usada, nxk_es_dividir, nxk_es_resto,
    nxk_es_leer_puerto, nxk_es_escribir_puerto, nxk_es_fallo, nxk_es_detener
};
int nxk_nxe_cargar(const unsigned char *p, size_t longitud) {
    /* Primero se valida el archivo completo: formato, arquitectura, limites y suma.
       Los codigos negativos indican rechazo; todavia no hay aislamiento de apps. */
    if (longitud < 64 || p[0]!='N' || p[1]!='X' || p[2]!='E' || p[3]) return -1;
    if (p[4]!=1 || p[5] || p[7] || p[6]!=(sizeof(void*)==4 ? 1 : 2)) return -2;
    if (u32(p+8)!=64 || u32(p+12)!=0x180000) return -3;
    uint32_t entrada=u32(p+16), carga=u32(p+20), memoria=u32(p+24), suma=2166136261u;
    if (!carga || memoria<carga || memoria>128*1024 || entrada>=carga || carga!=longitud-64) return -4;
    for (unsigned i=32; i<64; ++i) if (p[i]) return -5;
    for (uint32_t i=0; i<carga; ++i) suma=(suma ^ p[64+i])*16777619u;
    if (suma!=u32(p+28)) return -6;
    unsigned char *destino=(unsigned char *)(uintptr_t)0x180000;
    /* La zona sin datos del archivo empieza en cero. En x86, ambas operaciones
       usan ahora nuestras rutinas NEXTRY ASM. El archivo embebido no se solapa. */
    memset(destino, 0, memoria);
    memcpy(destino, p+64, carga);
    uint32_t (*ejecutar)(const NXE_API *)=(void *)(uintptr_t)(0x180000+entrada);
    return (int)ejecutar(&api);
}
int nxk_nxe_demo(void) {
    return nxk_nxe_cargar(nxe_demo_inicio, (uintptr_t)nxe_demo_fin-(uintptr_t)nxe_demo_inicio);
}
