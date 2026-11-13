/* Consola grafica experimental: framebuffer VGA BIOS13h, una CPU. */
#ifndef NXK_INTERFAZ_H
#define NXK_INTERFAZ_H
void nxk_interfaz_iniciar(void);
void nxk_interfaz_limpiar(void);
void nxk_interfaz_caracter(char c);
#endif
