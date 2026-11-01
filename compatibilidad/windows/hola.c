/* GUIA DE LECTURA NXK - compatibilidad/windows/hola.c
 * Programa Windows de demostracion que usa solo tres funciones Win32 admitidas por NXK.
 * Contexto y orden de estudio: ver LEER-CODIGO.md en la carpeta principal.
 */
/* EXE Win32 de prueba: usa exclusivamente tres funciones de KERNEL32.DLL. */
typedef unsigned long DWORD;
typedef void *HANDLE;
__declspec(dllimport) HANDLE __stdcall GetStdHandle(DWORD tipo);
__declspec(dllimport) int __stdcall WriteFile(HANDLE archivo, const void *datos, DWORD cantidad, DWORD *escritos, void *asincrono);
__declspec(dllimport) __attribute__((noreturn)) void __stdcall ExitProcess(DWORD codigo);
void inicio(void) {
    static const char texto[] = "Hola desde un EXE Win32 dentro de NXK\n";
    DWORD escritos=0;
    int correcto=WriteFile(GetStdHandle((DWORD)-11), texto, sizeof(texto)-1, &escritos, (void *)0);
    ExitProcess(correcto && escritos==sizeof(texto)-1 ? 0 : 1);
}
