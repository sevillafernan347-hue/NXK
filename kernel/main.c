/* GUIA DE LECTURA NXK - kernel/main.c
 * Coordina el kernel: consola VGA/serie, teclado, servicios para NEXTRY, pruebas y comandos.
 * Contexto y orden de estudio: ver LEER-CODIGO.md en la carpeta principal.
 */
#include "nxk.h"
#include "nxk_es.h"
#include "nxe.h"
#include "pe.h"
#include "paginas.h"
#include "archivos.h"
#if defined(__i386__)
#define NXK_ARCH_TEXT "x86 de 32 bits"
#define NXK_MODE_MARKER "NXK: PROTECTED MODE 32 OK\n"
#else
#define NXK_ARCH_TEXT "x86-64"
#define NXK_MODE_MARKER "NXK: LONG MODE OK\n"
#endif
static inline void out8(uint16_t port, uint8_t value) {
    __asm__ volatile("outb %0, %1" :: "a"(value), "Nd"(port));
}
static inline uint8_t in8(uint16_t port) {
    uint8_t value;
    __asm__ volatile("inb %1, %0" : "=a"(value) : "Nd"(port));
    return value;
}
extern int nxk_tareas_demo(void);
extern void nxk_reloj_iniciar(void);
extern uint32_t nxk_reloj_ticks(void);
static unsigned row, column;
static void serial_init(void) {
    /* Configura COM1: QEMU guarda lo que imprimimos aqui en el registro serie. */
    out8(0x3f9, 0); out8(0x3fb, 0x80);
    out8(0x3f8, 3); out8(0x3f9, 0);
    out8(0x3fb, 3); out8(0x3fa, 0xc7); out8(0x3fc, 0x0b);
}
static void put(char ch) {
    /* Cada caracter se envia al registro serie y a la pantalla de texto VGA.
       Al llegar abajo, las filas suben y queda libre la ultima fila. */
    while (!(in8(0x3fd) & 0x20)) {}
    out8(0x3f8, (uint8_t)ch);
    volatile uint16_t *vga = (volatile uint16_t *)0xb8000;
    if (ch == '\b') { if (column) { --column; vga[row*80+column] = 0x0b20; } }
    else if (ch == '\n') { column = 0; ++row; }
    else { vga[row * 80 + column] = 0x0b00 | (uint8_t)ch;
        if (++column == 80) { column = 0; ++row; } }
    if (row >= 25) {
        for (unsigned i=0; i<24*80; ++i) vga[i] = vga[i+80];
        for (unsigned i=24*80; i<25*80; ++i) vga[i] = 0x0b20;
        row = 24;
    }
}
static void print(const char *s) { while (*s) put(*s++); }
static void hexadecimal(uint32_t valor) {
    const char *digitos="0123456789ABCDEF";
    print("0x");
    for (int i=7;i>=0;--i) put(digitos[(valor>>(i*4))&15]);
}
void nxk_excepcion_fatal(const uint32_t *pila) {
    print("\nNXK: EXCEPCION CONTROLADA. CPU detenida.\n");
#if defined(__i386__)
    if (pila[8]==14) { uint32_t direccion; __asm__ volatile("mov %%cr2,%0":"=r"(direccion));
        print("CR2: "); hexadecimal(direccion); put('\n'); }
#endif
    /* PUSHAD: ocho registros; despues vector, error, EIP, CS y EFLAGS. */
    print("VECTOR: "); hexadecimal(pila[8]);
    print(" ERROR: "); hexadecimal(pila[9]);
    print(" EIP: "); hexadecimal(pila[10]); put('\n');
    print("EAX: "); hexadecimal(pila[7]); print(" EBX: "); hexadecimal(pila[4]);
    print(" ECX: "); hexadecimal(pila[6]); print(" EDX: "); hexadecimal(pila[5]); put('\n');
#ifdef NXK_EXCEPTION_TEST
    out8(0xf4, 0x12);
#endif
    for (;;) __asm__ volatile("cli; hlt");
}
/* Prefijo acotado por el terminador de la cadena de comando. */
static int prefijo(const char *s,const char *p) {
    while (*p) if (*s++!=*p++) return 0;
    return 1;
}
static char read_char(void) {
    /* Sondeo: consultamos si llego una tecla, sin interrupciones de teclado.
       El mapa PS/2 actual no interpreta Shift ni caracteres acentuados. */
    if (in8(0x3fd) & 1) return (char)in8(0x3f8);
    if (!(in8(0x64) & 1)) return 0;
    uint8_t code = in8(0x60);
    static const char keys[] = {
        0,0,'1','2','3','4','5','6','7','8','9','0','-','=', '\b','\t',
        'q','w','e','r','t','y','u','i','o','p','[',']','\n',0,'a','s',
        'd','f','g','h','j','k','l',';',39,96,0,92,'z','x','c','v',
        'b','n','m',',','.','/',0,'*',0,' '
    };
    return code < sizeof(keys) ? keys[code] : 0;
}
static void number(size_t n) {
    char b[24]; unsigned i = 0;
    do { b[i++] = '0' + n % 10; n /= 10; } while (n);
    while (i) put(b[--i]);
}
/* Puente pequeno entre NXK Espanol y las operaciones del nucleo. */
void nxk_es_mostrar(const char *texto) { print(texto); put('\n'); }
void nxk_pe_salida(const char *datos, uint32_t longitud) {
    while (longitud--) put(*datos++);
}
void nxk_es_numero(uint32_t valor) { number(valor); put('\n'); }
uint32_t nxk_es_memoria_usada(void) { return (uint32_t)nxk_heap_used(); }
_Noreturn void nxk_es_detener(void) {
    for (;;) __asm__ volatile("cli; hlt");
}
_Noreturn void nxk_es_fallo(uint32_t linea) {
    print("NXK Espanol: programa detenido por un error");
    if (linea) { print(" en la linea "); number(linea); }
    put('\n');
    nxk_es_detener();
}
uint32_t nxk_es_dividir(uint32_t a, uint32_t b) {
    if (!b) { print("Division entre cero.\n"); nxk_es_fallo(0); }
    return a / b;
}
uint32_t nxk_es_resto(uint32_t a, uint32_t b) {
    if (!b) { print("Resto con divisor cero.\n"); nxk_es_fallo(0); }
    return a % b;
}
uint32_t nxk_es_leer_puerto(uint32_t puerto) {
    if (puerto > 65535) { print("Puerto fuera de rango.\n"); nxk_es_fallo(0); }
    return in8((uint16_t)puerto);
}
void nxk_es_escribir_puerto(uint32_t puerto, uint32_t valor) {
    if (puerto > 65535 || valor > 255) {
        print("Puerto o byte fuera de rango.\n"); nxk_es_fallo(0);
    }
    out8((uint16_t)puerto, (uint8_t)valor);
}
void kernel_main(void) {
    /* Recorrido del arranque: consola -> excepciones -> memoria -> NEXTRY.
       La imagen de pruebas ejecuta comprobaciones; la normal abre la consola. */
    serial_init();
    extern void nxk_excepciones_iniciar(void);
    nxk_excepciones_iniciar();
#if defined(__i386__)
    extern int nxk_paginacion_iniciar(void);
    if (nxk_paginacion_iniciar()) { print("NXK: PAGINACION FAIL\n"); nxk_es_detener(); }
    print("NXK: PAGINACION Y WP OK\n");
#endif
#if defined(NXK_EXCEPTION_TEST) && defined(__i386__)
#if NXK_EXCEPTION_VECTOR == 14
    __asm__ volatile("movl $1,0" ::: "memory");
#elif NXK_EXCEPTION_VECTOR == 114
    *(volatile unsigned char *)(uintptr_t)kernel_main=0;
#elif NXK_EXCEPTION_VECTOR == 13
    __asm__ volatile("mov $0xfff8, %%ax; mov %%ax, %%ds" ::: "eax", "memory");
#elif NXK_EXCEPTION_VECTOR == 6
    __asm__ volatile("ud2");
#else
    __asm__ volatile("int3");
#endif
#endif
    volatile uint16_t *vga = (volatile uint16_t *)0xb8000;
    for (unsigned i=0; i<2000; ++i) vga[i] = 0x0b20;
    print("NXK 0.1 - kernel original " NXK_ARCH_TEXT "\n");
    print("NXK: RAM BIOS KiB: "); number(nxk_ram_kib()); put('\n');
#if defined(__i386__)
#ifdef NXK_TEST
    if (nxk_pagina_reservar() || nxk_pagina_liberar((void *)0x800000)!=-3 ||
        nxk_paginas_reservar_region(0x800000,4096)!=-3) {
        print("NXK: PAGINAS PREINICIO FAIL\n"); out8(0xf4,0x11); nxk_es_detener();
    }
    print("NXK: PAGINAS PREINICIO OK\n");
#endif
    if (nxk_paginas_bios()) print("NXK: gestor de paginas no disponible\n");
    print("NXK: PAGINAS LIBRES: "); number(nxk_paginas_libres()); put('\n');
#endif
    uint8_t *block = nxk_alloc(64);
    if (!block || block[0] || nxk_alloc(0x80001)) {
        print("NXK: MEMORY FAIL\n");
        for (;;) __asm__ volatile("hlt");
    }
    block[63] = 0xa5;
    print(NXK_MODE_MARKER "NXK: MEMORY OK\n");
    nxk_es_inicio();
    print("NXK: ESPANOL OK\n");
    nxk_reloj_iniciar();
#if defined(NXK_TEST) && defined(__i386__)
    /* HLT solo regresa si llega una interrupcion: verifica retornos y EOI. */
    while (nxk_reloj_ticks()<5) __asm__ volatile("hlt");
    print("NXK: RELOJ IRQ OK\n");
    if (nxk_tareas_demo()) { print("NXK: TAREAS FAIL\n"); out8(0xf4,0x11); nxk_es_detener(); }
    print("NXK: TAREAS COOPERATIVAS OK\n");
    extern int nxk_tareas_probar_limites(void);
    if (nxk_tareas_probar_limites()) { print("NXK: PILAS FAIL\n"); out8(0xf4,0x11); nxk_es_detener(); }
    print("NXK: PILAS Y REUTILIZACION OK\n");
#endif
#ifdef NXK_TEST
    if (nxk_archivos_probar()) { print("NXK: ARCHIVOS FAIL\n"); out8(0xf4,0x11); nxk_es_detener(); }
    print("NXK: ARCHIVOS RAM OK\n");
    unsigned char prueba_memoria[19];
    memset(prueba_memoria, 0xcc, sizeof(prueba_memoria));
    void *retorno_memoria=memset(prueba_memoria+1, 0x1a5, 17);
    memset(prueba_memoria+1, 0, 0);
    if (retorno_memoria!=prueba_memoria+1 || prueba_memoria[0]!=0xcc || prueba_memoria[18]!=0xcc)
        { print("NXK: LLENADO FAIL\n"); out8(0xf4,0x11); nxk_es_detener(); }
    for (unsigned i=1;i<18;++i) if (prueba_memoria[i]!=0xa5)
        { print("NXK: LLENADO FAIL\n"); out8(0xf4,0x11); nxk_es_detener(); }
    print("NXK: LLENADO OK\n");
    /* Llamada indirecta volatile: obliga a probar la rutina enlazada. */
    void *(*volatile copiar)(void *, const void *, size_t) = memcpy;
    unsigned char origen[260], destino[260];
    for (unsigned i=0; i<260; ++i) origen[i]=(unsigned char)(i*37);
    const unsigned longitudes[] = {0, 1, 3, 16, 17, 256};
    for (unsigned caso=0; caso<sizeof(longitudes)/sizeof(longitudes[0]); ++caso) {
        unsigned n=longitudes[caso];
        memset(destino, 0xcc, sizeof(destino));
        if (copiar(destino+1, origen+2, n)!=destino+1)
            { print("NXK: COPIA FAIL\n"); out8(0xf4,0x11); nxk_es_detener(); }
        for (unsigned i=0; i<260; ++i) {
            unsigned char esperado=(i>=1 && i<=n) ? origen[i+1] : 0xcc;
            if (destino[i]!=esperado || origen[i]!=(unsigned char)(i*37))
                { print("NXK: COPIA FAIL\n"); out8(0xf4,0x11); nxk_es_detener(); }
        }
    }
    print("NXK: COPIA OK\n");
    void *(*volatile mover)(void *, const void *, size_t) = memmove;
    /* Oraculo independiente: origen conserva una instantanea previa. */
    for (unsigned desde=0; desde<8; ++desde) {
        for (unsigned hacia=0; hacia<8; ++hacia) {
            for (unsigned n=0; n<=32; ++n) {
                for (unsigned i=0; i<64; ++i) origen[i]=destino[i]=(unsigned char)(i*37);
                if (mover(destino+hacia, destino+desde, n)!=destino+hacia)
                    { print("NXK: MOVER FAIL\n"); out8(0xf4,0x11); nxk_es_detener(); }
                for (unsigned i=0; i<64; ++i) {
                    unsigned char esperado=(i>=hacia && i<hacia+n) ? origen[desde+i-hacia] : origen[i];
                    if (destino[i]!=esperado)
                        { print("NXK: MOVER FAIL\n"); out8(0xf4,0x11); nxk_es_detener(); }
                }
            }
        }
    }
    print("NXK: MOVER OK\n");
    extern int nxk_probar_formatos(void);
    if (nxk_probar_formatos() != 0) { print("NXK: FORMATOS FAIL\n"); out8(0xf4, 0x11); nxk_es_detener(); }
    print("NXK: FORMATOS OK\n");
    if (nxk_nxe_demo() != 0) { print("NXK: NXE FAIL\n"); out8(0xf4, 0x11); nxk_es_detener(); }
    print("NXK: NXE OK\n");
#if defined(__i386__)
    if (nxk_pe_demo() != 0) { print("NXK: PE32 FAIL\n"); out8(0xf4, 0x11); nxk_es_detener(); }
    print("NXK: PE32 OK\n");
#endif
    /* Al final de la imagen de pruebas agotamos el heap deliberadamente.
       Los rechazos no deben cambiar su cursor ni los datos ya reservados. */
    size_t antes=nxk_heap_used(), disponible=nxk_heap_available();
    if (antes+disponible!=0x80000 || nxk_alloc((size_t)-1) || nxk_alloc(0) ||
        nxk_heap_used()!=antes || nxk_alloc(disponible+1) || nxk_heap_used()!=antes)
        { print("NXK: HEAP FAIL\n"); out8(0xf4,0x11); nxk_es_detener(); }
    unsigned char *ultimo=nxk_alloc(disponible);
    if (!ultimo || ((uintptr_t)ultimo & 15) || nxk_heap_available() ||
        nxk_heap_used()!=0x80000 || nxk_alloc(1) || block[63]!=0xa5)
        { print("NXK: HEAP FAIL\n"); out8(0xf4,0x11); nxk_es_detener(); }
    for (size_t i=0;i<disponible;++i) if (ultimo[i])
        { print("NXK: HEAP FAIL\n"); out8(0xf4,0x11); nxk_es_detener(); }
    print("NXK: HEAP LIMITES OK\n");
    extern int nxk_probar_paginas(void);
    int fallo_paginas=nxk_probar_paginas();
    if (fallo_paginas) {
        print("NXK: PAGINAS FAIL caso "); number((size_t)fallo_paginas); put('\n');
        out8(0xf4,0x11); nxk_es_detener();
    }
    print("NXK: PAGINAS OK\n");
    print("NXK: BOOT PASS\n");
    out8(0xf4, 0x10);
#endif
    print("Escribe help y pulsa Enter. Teclado basico sin Shift.\n");
    char line[80]; unsigned length = 0;
    /* Bucle de comandos: recoge una linea y ejecuta una accion al pulsar Enter.
       No es un planificador: aqui todavia corre una sola tarea del kernel. */
    print("nxk> ");
    for (;;) {
        char ch = read_char();
        if (!ch) { __asm__ volatile("pause"); continue; }
        if (ch == '\r' || ch == '\n') {
            put('\n'); line[length] = 0;
            if (nxk_equal(line, "help")) print("help | version | mem | tiempo | tareas | archivos | halt\nejecutar hola.nxe | ejecutar hola.exe\nescribir nombre texto | agregar nombre texto\nleer nombre | borrar nombre | renombrar nombre nuevo\n");
            else if (nxk_equal(line, "ejecutar hola.nxe")) {
                if (nxk_nxe_demo() != 0) print("No se pudo ejecutar el archivo NXE.\n");
            }
            else if (nxk_equal(line, "ejecutar hola.exe")) {
                if (nxk_pe_demo() != 0) print("EXE no compatible. El puente inicial requiere NXK de 32 bits.\n");
            }
            else if (nxk_equal(line, "version")) print("NXK 0.1, nucleo propio " NXK_ARCH_TEXT "\n");
            else if (prefijo(line,"escribir ") || prefijo(line,"agregar ")) {
                char *nombre=line+(line[0]=='a' ? 8 : 9), *texto=nombre;
                while (*texto && *texto!=' ') ++texto;
                if (!*texto) print("Uso: escribir nombre texto\n");
                else {
                    *texto++=0; size_t n=0; while (texto[n]) ++n;
                    print((line[0]=='a' ? nxk_archivo_agregar(nombre,texto,n) : nxk_archivo_escribir(nombre,texto,n)) ? "No se pudo escribir.\n" : "Archivo guardado en RAM.\n");
                }
            }
            else if (prefijo(line,"renombrar ")) {
                char *nombre=line+10,*nuevo=nombre;
                while (*nuevo && *nuevo!=' ') ++nuevo;
                if (!*nuevo) print("Uso: renombrar nombre nuevo\n");
                else { *nuevo++=0; print(nxk_archivo_renombrar(nombre,nuevo) ? "No se pudo renombrar.\n" : "Archivo renombrado.\n"); }
            }
            else if (prefijo(line,"leer ")) {
                char datos[256]; int n=nxk_archivo_leer(line+5,datos,sizeof(datos));
                if (n<0) print("No se pudo leer.\n");
                else { for (int i=0;i<n;++i) put(datos[i]); put('\n'); }
            }
            else if (prefijo(line,"borrar ")) {
                print(nxk_archivo_borrar(line+7) ? "No se pudo borrar.\n" : "Archivo borrado.\n");
            }
            else if (nxk_equal(line, "archivos")) {
                for (unsigned i=0;i<8;++i) { const char *s=nxk_archivo_nombre(i); if (s) { print(s); put('\n'); } }
            }
            else if (nxk_equal(line, "tareas")) {
                print(nxk_tareas_demo() ? "Tareas no disponibles o fallo.\n" : "Dos tareas: pilas propias, esperas PIT y finalizacion verificadas.\n");
            }
            else if (nxk_equal(line, "tiempo")) {
                print("Ticks PIT (~100 Hz; contador de 32 bits): "); number(nxk_reloj_ticks()); put('\n');
            }
            else if (nxk_equal(line, "mem")) {
                print("Heap fijo usado: "); number(nxk_heap_used()); print(" / 524288 bytes\n");
                print("Heap disponible: "); number(nxk_heap_available()); print(" bytes\n");
                print("RAM utilizable descrita por BIOS: "); number(nxk_ram_kib());
                print(" KiB (0 = mapa no disponible; no es RAM libre)\n");
                print("Paginas fisicas libres/total (4 KiB): "); number(nxk_paginas_libres());
                print(" / "); number(nxk_paginas_total()); put('\n');
            } else if (nxk_equal(line, "halt")) {
                print("NXK detenido. Puedes cerrar QEMU.\n");
                for (;;) __asm__ volatile("cli; hlt");
            } else if (length) print("Comando desconocido. Usa help.\n");
            length = 0; print("nxk> ");
        } else if ((ch == '\b' || ch == 127) && length) {
            --length; put('\b');
        } else if (ch >= 32 && ch < 127 && length < sizeof(line)-1) {
            line[length++] = ch; put(ch);
        }
    }
}
