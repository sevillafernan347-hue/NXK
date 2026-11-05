/* Instantaneas NXPD1: disco dedicado IDE esclavo, 8 archivos RAM de 256 B.
 * Solo consola/kernel exterior a tareas, una CPU, fuera de IRQ. */
#ifndef NXK_PERSISTENCIA_H
#define NXK_PERSISTENCIA_H
#include "nxk.h"
/* 0 exito, 1 disco vacio al cargar; negativos: disco/formato/corrupcion/E/S,
 * generacion agotada, RAM invalida/sin memoria, contexto de tarea. */
int nxk_persistencia_guardar(void);
int nxk_persistencia_cargar(void);
#endif
