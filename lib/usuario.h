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
#endif
