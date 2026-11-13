/* Memoria virtual privada x86: base para futuros procesos, no planificador.
 * Puente C, una CPU, fuera de IRQ y de tareas; punteros kernel confiables. */
#ifndef NXK_ESPACIOS_H
#define NXK_ESPACIOS_H
#include "nxk.h"
#define NXK_ESPACIOS_MAX 4u
#define NXK_ESPACIO_CODIGO 0x400000u
#define NXK_ESPACIO_PILA 0x401000u
#define NXK_ESPACIO_DATOS 0x402000u
#define NXK_ESPACIO_DATOS_MAX 4u
struct nxk_espacio_info { uint32_t id, directorio, codigo_fisico, pila_fisica, paginas_datos, cuota_datos; };
/* Cuota inicial4 por generacion, solo reducciones0..4; no retira paginas.
 * 0 exito,-1 ID/contrato,-2 inferior al uso,-3 contexto,-5 ampliacion.
 * Solo CR3 kernel o propio, fuera de tarea/IRQ. Consulta conserva salida
 * al fallar;0 es una cuota valida. No incluye las cinco paginas base. */
int nxk_espacio_limitar_datos(uint32_t id,uint32_t cuota);
int nxk_espacio_cuota(uint32_t id,uint32_t *salida);
/* Crear: 1..4096 bytes, codigo RO, pila RW en cero; salida intacta al fallar.
 * 0 exito; -1 contrato/ID; -2 tabla llena/activo al destruir;
 * -3 contexto no permitido; -4 memoria fisica insuficiente. */
int nxk_espacio_crear(const void *codigo,size_t longitud,uint32_t *id);
int nxk_espacio_consultar(uint32_t id,struct nxk_espacio_info *salida);
/* Slot0..3: -2 vacio; -1 contrato. Errores conservan salida. */
int nxk_espacio_enumerar(unsigned slot,struct nxk_espacio_info *salida);
/* Una pagina RW propia de 4096 bytes, en cero, hasta cuatro por espacio.
 * Elige la primera direccion libre; salida intacta al rechazar.
 * 0 exito; -1 ID/null, -2 cuota, -3 contexto, -4 sin memoria fisica.
 * Solo CR3 kernel o el de este ID, fuera de tareas/IRQ; no hay reservas implicitas. */
int nxk_espacio_reservar_pagina(uint32_t id,uint32_t *direccion);
/* Lote1..4 en VA contigua (fisicas independientes), sin publicacion parcial.
 * Mismos errores de reservar_pagina; -2 incluye fragmentacion virtual/cuota.
 * Salida intacta al fallar. Liberar valida todo antes de tocar mapas/datos;
 * -1 rango/hueco,-3 contexto,-4 asignador corrupto,-5 alguna pagina retenida. */
int nxk_espacio_reservar_paginas(uint32_t id,uint32_t cantidad,uint32_t *direccion);
int nxk_espacio_liberar_paginas(uint32_t id,uint32_t direccion,uint32_t cantidad);
/* Direccion inicial exacta de una pagina reservada; -1 ID/direccion/reserva,
 * -3 contexto, -4 fallo del gestor fisico, -5 buffer IPC pendiente sobre ella.
 * Invalida el TLB antes de devolver. */
int nxk_espacio_liberar_pagina(uint32_t id,uint32_t direccion);
/* Comprueba todo el rango sin copiar ni modificar datos. Solo CR3 kernel o
 * el del mismo espacio, fuera de tarea/IRQ; escritura debe ser0 o1.
 * 0 exito; -1 ID/bandera, -2 rango/hueco, -3 contexto. n0 permite huecos
 * desde el minimo (codigo para leer/pila para escribir) hasta0x406000. */
int nxk_espacio_validar_rango(uint32_t id,uint32_t virtual,size_t longitud,int escritura);
/* Copias solo desde CR3 kernel, buffers kernel confiables sin solaparse con
 * tablas ni codigo del espacio, ni con sus paginas adicionales. Leer codigo,
 * pila o datos reservados; escribir pila o datos. Valida tambien todos los
 * huecos antes de copiar, aunque el respaldo fisico no sea contiguo.
 * 0 exito, -1 ID/buffer, -2 rango, -3 contexto. Rechazo no copia parcialmente.
 * Longitud cero admite null y cualquier inicio desde el minimo permitido
 * hasta 0x406000 inclusive, incluso si no hay datos reservados. */
int nxk_espacio_leer(uint32_t id,uint32_t virtual,void *destino,size_t longitud);
int nxk_espacio_escribir(uint32_t id,uint32_t virtual,const void *origen,size_t longitud);
/* Activa desde kernel u otro espacio propio, refresca mapas kernel y CR3.
 * NO cambia registros CPU, TSS ni pila de retorno de un proceso. */
int nxk_espacio_activar(uint32_t id);
int nxk_espacio_restaurar(void);
/* Destruir: -5 si un observador de proceso retiene el resultado de este ID.
 * Debe consumir su espera o cancelarla antes de liberar el destino. */
int nxk_espacio_destruir(uint32_t id);
int nxk_espacios_demo(void);
#ifdef NXK_TEST
int nxk_espacios_probar(void);
#endif
#endif
