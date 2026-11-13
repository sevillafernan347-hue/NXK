/* Interfaz original NXK: pixeles en VGA320x200 y fuente propia5x7.
 * El BIOS selecciona modo13h antes de entrar en modo protegido. No hay
 * raton ni gestor de ventanas: la consola real del kernel recibe teclado.
 * Todas las coordenadas y el buffer de texto estan acotados. */
#include "nxk.h"
#include "interfaz.h"
#ifdef NXK_INTERFAZ
#define COLS 50u
#define FILAS 13u
static char texto[FILAS][COLS];
static unsigned fila,columna;
static volatile unsigned char *const video=(void *)0xa0000;
/* Cada fila contiene cinco bits; dibujos originales, sin fuentes externas. */
static const unsigned char letras[36][7]={
 {14,17,19,21,25,17,14},{4,12,4,4,4,4,14},{14,17,1,2,4,8,31},
 {30,1,1,14,1,1,30},{2,6,10,18,31,2,2},{31,16,16,30,1,1,30},
 {14,16,16,30,17,17,14},{31,1,2,4,8,8,8},{14,17,17,14,17,17,14},
 {14,17,17,15,1,1,14},
 {14,17,17,31,17,17,17},{30,17,17,30,17,17,30},{14,17,16,16,16,17,14},
 {30,17,17,17,17,17,30},{31,16,16,30,16,16,31},{31,16,16,30,16,16,16},
 {14,17,16,23,17,17,15},{17,17,17,31,17,17,17},{14,4,4,4,4,4,14},
 {7,2,2,2,2,18,12},{17,18,20,24,20,18,17},{16,16,16,16,16,16,31},
 {17,27,21,21,17,17,17},{17,25,25,21,19,19,17},{14,17,17,17,17,17,14},
 {30,17,17,30,16,16,16},{14,17,17,17,21,18,13},{30,17,17,30,20,18,17},
 {15,16,16,14,1,1,30},{31,4,4,4,4,4,4},{17,17,17,17,17,17,14},
 {17,17,17,17,17,10,4},{17,17,17,21,21,21,10},{17,17,10,4,10,17,17},
 {17,17,10,4,4,4,4},{31,1,2,4,8,16,31}
};
static unsigned bits(char c,unsigned y) {
    if (c>='a' && c<='z') c-=32;
    if (c>='0' && c<='9') return letras[c-'0'][y];
    if (c>='A' && c<='Z') return letras[10+c-'A'][y];
    switch (c) {
      case ' ':return 0;
      case '.':return y==6?4:0;
      case ',':return y==5?4:y==6?8:0;
      case ':':return y==2 || y==5?4:0;
      case ';':return y==2 || y==5?4:y==6?8:0;
      case '-':return y==3?14:0;
      case '_':return y==6?31:0;
      case '=':return y==2 || y==4?31:0;
      case '+':return y==3?31:y>0 && y<6?4:0;
      case '/':return 1u<<(y*4/6);
      case '\\':return 16u>>(y*4/6);
      case '|':return 4;
      case '>':return 16u>>(y<4?y:6-y);
      case '<':return 1u<<(y<4?y:6-y);
      case '[':return y==0 || y==6?14:8;
      case ']':return y==0 || y==6?14:2;
      case '(':return y==0 || y==6?2:4;
      case ')':return y==0 || y==6?8:4;
      case '!':return y<4 || y==6?4:0;
      case '"':return y<2?10:0;
      case '\'':return y<2?4:0;
      default:return y==0 || y==6?31:17;
    }
}
static void rect(unsigned x,unsigned y,unsigned w,unsigned h,unsigned color) {
    for (unsigned j=y;j<y+h && j<200;++j)
        for (unsigned i=x;i<x+w && i<320;++i) video[j*320+i]=(unsigned char)color;
}
static void glifo(unsigned x,unsigned y,char c,unsigned color) {
    for (unsigned j=0;j<7 && y+j<200;++j) {
        unsigned b=bits(c,j);
        for (unsigned i=0;i<5 && x+i<320;++i)
            if (b&(16u>>i)) video[(y+j)*320+x+i]=(unsigned char)color;
    }
}
static void etiqueta(unsigned x,unsigned y,const char *s,unsigned color) {
    while (*s && x+5<320) { glifo(x,y,*s++,color); x+=6; }
}
static void celda(unsigned f,unsigned c) {
    rect(10+c*6,70+f*8,6,8,0); glifo(10+c*6,70+f*8,texto[f][c],6);
}
static void cursor(void) { rect(10+columna*6,77+fila*8,5,1,5); }
void nxk_interfaz_limpiar(void) {
    memset(texto,' ',sizeof(texto)); fila=columna=0;
    rect(10,70,300,104,0); cursor();
}
void nxk_interfaz_iniciar(void) {
    /* Paleta RGB de seis bits por componente; indices propios0..7. */
    const unsigned char paleta[][3]={{3,5,9},{5,9,16},{9,16,25},{13,25,35},
        {14,42,50},{22,55,43},{53,58,61},{63,63,63}};
    __asm__ volatile("outb %0,%1"::"a"((unsigned char)0),"Nd"((uint16_t)0x3c8));
    for (unsigned i=0;i<sizeof(paleta);++i)
        __asm__ volatile("outb %0,%1"::"a"(((const unsigned char *)paleta)[i]),"Nd"((uint16_t)0x3c9));
    rect(0,0,320,200,1); rect(0,0,320,24,2);
    etiqueta(10,8,"NXK",7); etiqueta(40,8,"/ MINI SISTEMA",5);
    etiqueta(226,8,"EXPERIMENTO",6);
    rect(8,29,148,16,2); rect(164,29,148,16,2);
    etiqueta(16,34,"X86 / 32 BITS",6); etiqueta(172,34,"NEXTRY + NXA",5);
    rect(8,52,304,126,3); rect(9,53,302,14,2);
    etiqueta(16,57,"TERMINAL DEL SISTEMA",7);
    rect(293,57,5,5,4); rect(302,57,5,5,5);
    rect(0,184,320,16,2); etiqueta(10,189,"HELP  MEM  ARCHIVOS  PROCESO DEMO",6);
    nxk_interfaz_limpiar();
}
void nxk_interfaz_caracter(char c) {
    celda(fila,columna); /* Quitar el cursor anterior. */
    if (c=='\b') {
        if (columna) --columna;
        else if (fila) { --fila; columna=COLS-1; }
        texto[fila][columna]=' '; celda(fila,columna);
    } else if (c=='\r') columna=0;
    else if (c=='\n') { columna=0; ++fila; }
    else {
        texto[fila][columna]=c; celda(fila,columna);
        if (++columna==COLS) { columna=0; ++fila; }
    }
    if (fila==FILAS) {
        memmove(texto[0],texto[1],(FILAS-1)*COLS);
        memset(texto[FILAS-1],' ',COLS); fila=FILAS-1;
        for (unsigned f=0;f<FILAS;++f) for (unsigned col=0;col<COLS;++col) celda(f,col);
    }
    cursor();
}
#endif
