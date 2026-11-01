# GUIA DE LECTURA NXK - build.ps1
# Construye el kernel: compila NEXTRY, ensambla NXA, enlaza modulos y crea la imagen de disco.
# Contexto y orden de estudio: ver LEER-CODIGO.md en la carpeta principal.
param(
    [switch]$Test,
    [switch]$ExceptionTest,
    [ValidateSet(3,6,13)][int]$ExceptionVector = 3,
    [ValidateSet('x86','x64')][string]$Architecture = 'x86'
)
$ErrorActionPreference = 'Stop'
Set-Location $PSScriptRoot
$zig = Join-Path $PSScriptRoot 'tools\zig\zig.exe'
$python = Join-Path $PSScriptRoot 'tools\python\python.exe'
if (!(Test-Path $zig)) { throw 'Falta tools\zig\zig.exe. Consulta LEEME.md.' }
if (!(Test-Path $python)) { throw 'Falta el compilador de NXK Espanol: tools\python\python.exe.' }
$buildDir = if ($Architecture -eq 'x86') { 'build/x86' } else { 'build' }
$target = if ($Architecture -eq 'x86') { 'x86-freestanding-none' } else { 'x86_64-freestanding-none' }
$bootSource = if ($Architecture -eq 'x86') { 'boot/boot32.S' } else { 'boot/boot.S' }
$entrySource = if ($Architecture -eq 'x86') { 'kernel/entry32.S' } else { 'kernel/entry.S' }
New-Item -ItemType Directory -Force $buildDir | Out-Null
& $python lenguaje/compilador.py kernel/inicio.nxf -o "$buildDir/inicio_nxe.c"
if ($LASTEXITCODE -ne 0) { throw 'NXK Espanol encontro errores. Corrige kernel\inicio.nxf.' }
& "$PSScriptRoot/compilar-app.ps1" -Architecture $Architecture -Salida "$buildDir/hola.nxe"
if ($Architecture -eq 'x86') {
    & $zig cc -target x86-windows-gnu -ffreestanding -fno-stack-protector -fno-builtin -mno-sse -mno-sse2 -Os -nostdlib compatibilidad/windows/hola.c '-Wl,--entry,inicio' '-Wl,--subsystem,console' '-Wl,--image-base,0x400000' -lkernel32 -o "$buildDir/hola.exe"
    if ($LASTEXITCODE -ne 0) { throw 'No se pudo crear el EXE Win32 de prueba.' }
}
$asm = ".section .rodata`n.global nxe_demo_inicio, nxe_demo_fin`nnxe_demo_inicio:`n.incbin `"$buildDir/hola.nxe`"`nnxe_demo_fin:`n.section .note.GNU-stack,`"`",@progbits`n"
if ($Architecture -eq 'x86') { $asm += ".section .rodata`n.global pe_demo_inicio, pe_demo_fin`npe_demo_inicio:`n.incbin `"$buildDir/hola.exe`"`npe_demo_fin:`n" }
if ($Architecture -eq 'x86') {
    & $python lenguaje/ensamblador.py kernel/tareas32.nxa -o "$buildDir/tareas.bin"
    if ($LASTEXITCODE -ne 0) { throw 'Error ensamblando tareas.' }
    $asm += ".section .text`n.global nxk_cambiar_tarea`nnxk_cambiar_tarea:`n.incbin `"$buildDir/tareas.bin`"`n"
    & $python lenguaje/ensamblador.py kernel/reloj32.nxa -o "$buildDir/reloj.bin"
    if ($LASTEXITCODE -ne 0) { throw 'Error ensamblando IRQ0 con NEXTRY ASM.' }
    $asm += ".section .rodata`n.global reloj_nxa_inicio, reloj_nxa_fin`nreloj_nxa_inicio:`n.incbin `"$buildDir/reloj.bin`"`nreloj_nxa_fin:`n"
    & $python lenguaje/ensamblador.py kernel/excepciones32.nxa -o "$buildDir/excepciones.bin"
    if ($LASTEXITCODE -ne 0) { throw 'Error al ensamblar las excepciones con NEXTRY ASM.' }
    $asm += ".section .rodata`n.global excepcion_nxa_inicio, excepcion_nxa_fin`nexcepcion_nxa_inicio:`n.incbin `"$buildDir/excepciones.bin`"`nexcepcion_nxa_fin:`n"
}
if ($Architecture -eq 'x86') {
    & $python lenguaje/ensamblador.py lib/llenar32.nxa -o "$buildDir/llenar32.bin"
    if ($LASTEXITCODE -ne 0) { throw 'Error ensamblando memoria con NEXTRY ASM.' }
    $asm += ".section .text`n.global memset`nmemset:`n.incbin `"$buildDir/llenar32.bin`"`n"
    $asm += '# MEMORIA ' + (Get-FileHash "$buildDir/llenar32.bin" -Algorithm SHA256).Hash + "`n"
    & $python lenguaje/ensamblador.py lib/copiar32.nxa -o "$buildDir/copiar32.bin"
    if ($LASTEXITCODE -ne 0) { throw 'Error ensamblando copia con NEXTRY ASM.' }
    $asm += ".section .text`n.global memcpy`nmemcpy:`n.incbin `"$buildDir/copiar32.bin`"`n"
    $asm += '# COPIA ' + (Get-FileHash "$buildDir/copiar32.bin" -Algorithm SHA256).Hash + "`n"
    & $python lenguaje/ensamblador.py lib/mover32.nxa -o "$buildDir/mover32.bin"
    if ($LASTEXITCODE -ne 0) { throw 'Error ensamblando movimiento con NEXTRY ASM.' }
    $asm += ".section .text`n.global memmove`nmemmove:`n.incbin `"$buildDir/mover32.bin`"`n"
    $asm += '# MOVER ' + (Get-FileHash "$buildDir/mover32.bin" -Algorithm SHA256).Hash + "`n"
}
$asm += '# NXE ' + (Get-FileHash "$buildDir/hola.nxe" -Algorithm SHA256).Hash + "`n"
if ($Architecture -eq 'x86') {
    $asm += '# PE ' + (Get-FileHash "$buildDir/hola.exe" -Algorithm SHA256).Hash + "`n"
    $asm += '# EXCEPCIONES ' + (Get-FileHash "$buildDir/excepciones.bin" -Algorithm SHA256).Hash + "`n"
}
[IO.File]::WriteAllText((Join-Path $PSScriptRoot "$buildDir/app_embebida.S"), $asm)
function Invoke-Zig { & $zig @args; if ($LASTEXITCODE -ne 0) { throw "Zig fallo: $args" } }
if ($Architecture -eq 'x86') {
    & $python lenguaje/ensamblador.py boot/arranque32.nxa -o "$buildDir/boot.bin"
    if ($LASTEXITCODE -ne 0) { throw 'Error en NEXTRY ASM.' }
} else {
    Invoke-Zig cc -target $target -c $bootSource -o "$buildDir/boot.o"
    Invoke-Zig ld.lld -T boot/boot.ld --oformat=binary "$buildDir/boot.o" -o "$buildDir/boot.bin"
}
if ($Architecture -eq 'x86') {
    & $python lenguaje/ensamblador.py kernel/entrada32.nxa --permitir-externos -o "$buildDir/entrada.bin"
    if ($LASTEXITCODE -ne 0) { throw 'Error en la entrada NEXTRY ASM.' }
    $entrada = ".section .text.entry`n.global kernel_entry`nkernel_entry:`n.incbin `"$buildDir/entrada.bin`"`n.section .note.GNU-stack,`"`",@progbits`n"
    [IO.File]::WriteAllText((Join-Path $PSScriptRoot "$buildDir/entrada_objeto.S"),$entrada)
    $entrySource = "$buildDir/entrada_objeto.S"
}
Invoke-Zig cc -target $target -c $entrySource -o "$buildDir/entry.o"
$flags = @('-target',$target,'-ffreestanding','-fno-stack-protector','-fno-pic','-mno-sse','-mno-sse2','-mno-mmx','-fno-vectorize','-fno-slp-vectorize','-Os','-Wall','-Wextra','-Werror','-Ilib')
if ($Architecture -eq 'x64') { $flags += '-mno-red-zone' }
if ($Test) { $flags += '-DNXK_TEST' }
if ($ExceptionTest) { $flags += '-DNXK_EXCEPTION_TEST' }
if ($ExceptionTest) { $flags += "-DNXK_EXCEPTION_VECTOR=$ExceptionVector" }
Invoke-Zig cc @flags -c kernel/tareas.c -o "$buildDir/tareas.o"
Invoke-Zig cc @flags -c kernel/archivos.c -o "$buildDir/archivos.o"
Invoke-Zig cc @flags -c kernel/main.c -o "$buildDir/main.o"
Invoke-Zig cc @flags -c lib/memory.c -o "$buildDir/memory.o"
Invoke-Zig cc @flags -c "$buildDir/inicio_nxe.c" -o "$buildDir/inicio_nxe.o"
Invoke-Zig cc @flags -c kernel/nxe.c -o "$buildDir/nxe.o"
Invoke-Zig cc @flags -c kernel/excepciones.c -o "$buildDir/excepciones.o"
Invoke-Zig cc @flags -c kernel/ram.c -o "$buildDir/ram.o"
Invoke-Zig cc @flags -c kernel/paginas.c -o "$buildDir/paginas.o"
Invoke-Zig cc @flags -c compatibilidad/windows/pe.c -o "$buildDir/pe.o"
Invoke-Zig cc -target $target -c "$buildDir/app_embebida.S" -o "$buildDir/app_embebida.o"
$objects=@("$buildDir/entry.o","$buildDir/main.o","$buildDir/memory.o","$buildDir/inicio_nxe.o","$buildDir/nxe.o","$buildDir/pe.o","$buildDir/app_embebida.o")
$objects += "$buildDir/excepciones.o"
$objects += "$buildDir/tareas.o"
$objects += "$buildDir/archivos.o"
$objects += "$buildDir/ram.o"
$objects += "$buildDir/paginas.o"
if ($Architecture -eq 'x86') {
    Invoke-Zig cc -target $target -c compatibilidad/windows/retorno32.S -o "$buildDir/pe_retorno.o"
    $objects += "$buildDir/pe_retorno.o"
}
if ($Test) {
    Invoke-Zig cc @flags -c kernel/pruebas_cargadores.c -o "$buildDir/pruebas_cargadores.o"
    $objects += "$buildDir/pruebas_cargadores.o"
    Invoke-Zig cc @flags -c kernel/pruebas_paginas.c -o "$buildDir/pruebas_paginas.o"
    $objects += "$buildDir/pruebas_paginas.o"
}
Invoke-Zig ld.lld -T kernel/linker.ld @objects -o "$buildDir/kernel.elf"
if ($Architecture -eq 'x86') {
    & $python lenguaje/simbolos_elf.py "$buildDir/kernel.elf" "$buildDir/simbolos-entrada.json"
    if ($LASTEXITCODE -ne 0) { throw 'No se resolvieron los simbolos del nucleo.' }
    & $python lenguaje/ensamblador.py kernel/entrada32.nxa --simbolos "$buildDir/simbolos-entrada.json" -o "$buildDir/entrada.bin"
    if ($LASTEXITCODE -ne 0) { throw 'Error resolviendo la entrada NEXTRY ASM.' }
    # La huella cambia al resolver direcciones: evita reutilizar el objeto provisional.
    $entradaFinal = $entrada + '# ' + [IO.File]::ReadAllText((Join-Path $PSScriptRoot "$buildDir/simbolos-entrada.json")) + "`n"
    [IO.File]::WriteAllText((Join-Path $PSScriptRoot $entrySource),$entradaFinal)
    Invoke-Zig cc -target $target -c $entrySource -o "$buildDir/entry.o"
    Invoke-Zig ld.lld -T kernel/linker.ld @objects -o "$buildDir/kernel.elf"
    & $python lenguaje/simbolos_elf.py "$buildDir/kernel.elf" "$buildDir/simbolos-entrada.json" --comprobar
    if ($LASTEXITCODE -ne 0) { throw 'Las direcciones del enlace no son estables.' }
}
Invoke-Zig ld.lld -T kernel/linker.ld --oformat=binary @objects -o "$buildDir/kernel.bin"
$boot = [IO.File]::ReadAllBytes((Join-Path $PWD "$buildDir/boot.bin"))
$kernel = [IO.File]::ReadAllBytes((Join-Path $PWD "$buildDir/kernel.bin"))
if ($boot.Length -ne 512 -or $boot[510] -ne 0x55 -or $boot[511] -ne 0xaa) { throw 'Sector de arranque invalido' }
if ($kernel.Length -gt 32768) { throw 'Kernel demasiado grande para el cargador inicial' }
$disk = New-Object byte[] (1024*1024)
[Array]::Copy($boot,0,$disk,0,$boot.Length)
[Array]::Copy($kernel,0,$disk,512,$kernel.Length)
$name = if ($Test) { 'nxk-test.img' } else { 'nxk.img' }
[IO.File]::WriteAllBytes((Join-Path $PWD "$buildDir/$name"),$disk)
Write-Host "NXK $Architecture compilado: $buildDir/$name ($($kernel.Length) bytes de kernel)"

