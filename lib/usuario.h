/* Diagnosticos de la demostracion sincronica, solo kernel. Copias de registros.
 * Historial fijo de ocho entradas, indice 0 es la mas reciente. */
#ifndef NXK_USUARIO_H
#define NXK_USUARIO_H
#include "nxk.h"
struct nxk_usuario_resultado {
    uint32_t secuencia, salida, fallo, vector, error, eip, direccion, pila;
};
unsigned nxk_usuario_historial_cantidad(void);
int nxk_usuario_historial(unsigned reciente,struct nxk_usuario_resultado *salida);
int nxk_usuario_caso_nulo(void);
int nxk_usuario_caso_bucle(void);
int nxk_usuario_probar_servicios(void);
int nxk_usuario_nextry(void);
int nxk_usuario_cargar_nxe(const unsigned char *datos,size_t longitud);
int nxk_usuario_ejecutar_archivo(const char *nombre);
/* Turno sincrono en espacio ya creado. Comienza en 400000 con ESP402000 y
 * registros limpios; conserva bytes de pila. No reanuda EIP ni planifica.
 * 0 resultado disponible (fallo indica excepcion/PIT); negativos conservan
 * salida: -1 contrato/ID/activo, -2 contexto, -3 heap, -4 plantilla, -5 retorno.
 * Solo kernel CR3 original, IF habilitada, GDT/TSS/PIT iniciados, fuera de tareas. */
int nxk_usuario_ejecutar_espacio(uint32_t id,struct nxk_usuario_resultado *salida);
enum nxk_usuario_motivo { NXK_USUARIO_FINAL, NXK_USUARIO_CEDER, NXK_USUARIO_QUANTUM };
/* Marco17 DWORD de INT81/IRQ0; API interna kernel. Solo publica marco al
 * suspender y retorna motivo 1=ceder, 2=quantum, 0=salida/fallo.
 * -6 rechaza EIP/ESP/selectores antes de IRET.
 * No sustituye NXE ABI: reservado al gestor de procesos, buffers confiables. */
int nxk_usuario_reanudar_espacio(uint32_t id,uint32_t *marco,struct nxk_usuario_resultado *salida,uint32_t *cedido);
/* Validar NXE2 y copiar a espacio retenido, sin ejecutar. Salida ID intacta
 * ante error. El llamador destruye el espacio tras terminar de usarlo. */
int nxk_usuario_crear_espacio_nxe(const unsigned char *datos,size_t longitud,uint32_t *id);
int nxk_usuario_preparar_archivo(const char *nombre,uint32_t *id);
int nxk_usuario_espacios_demo(void);
#ifdef NXK_TEST
int nxk_usuario_espacios_probar(void);
#endif
#endif
