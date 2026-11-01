# GUIA DE LECTURA NXK - compilar-app.ps1
# Transforma una app NEXTRY o NXJ en codigo nativo y la empaqueta como archivo NXE.
# Contexto y orden de estudio: ver LEER-CODIGO.md en la carpeta principal.
param(
    [string]$Fuente = 'lenguaje/ejemplos/hola.nxf',
    [string]$Salida = '',
    [ValidateSet('x86','x64')][string]$Architecture = 'x86'
)
$ErrorActionPreference = 'Stop'
Set-Location $PSScriptRoot
if (!$Salida) { $Salida = if ($Architecture -eq 'x86') { 'build/x86/hola.nxe' } else { 'build/hola.nxe' } }
$dir = Join-Path (Split-Path $Salida -Parent) (([IO.Path]::GetFileNameWithoutExtension($Salida)) + '-intermedios')
New-Item -ItemType Directory -Force $dir | Out-Null
$python = Join-Path $PSScriptRoot 'tools/python/python.exe'
$zig = Join-Path $PSScriptRoot 'tools/zig/zig.exe'
$target = if ($Architecture -eq 'x86') { 'x86-freestanding-none' } else { 'x86_64-freestanding-none' }
function Ejecutar-Zig { & $zig @args; if ($LASTEXITCODE -ne 0) { throw 'No se pudo construir la aplicacion NXE.' } }
$compilador = if ([IO.Path]::GetExtension($Fuente) -eq '.nxj') { 'lenguaje/nxj.py' } else { 'lenguaje/compilador.py' }
& $python $compilador $Fuente -o "$dir/programa.c"
if ($LASTEXITCODE -ne 0) { throw 'La fuente contiene errores de NXK Espanol.' }
$flags=@('-target',$target,'-ffreestanding','-fno-stack-protector','-fno-pic','-mno-sse','-mno-sse2','-mno-mmx','-fno-vectorize','-fno-slp-vectorize','-O2','-Wall','-Wextra','-Werror','-Ilib')
if ($Architecture -eq 'x64') { $flags += '-mno-red-zone' }
Ejecutar-Zig cc @flags -c "$dir/programa.c" -o "$dir/programa.o"
Ejecutar-Zig cc @flags -c lenguaje/runtime_nxe.c -o "$dir/runtime.o"
Ejecutar-Zig ld.lld -T lenguaje/aplicacion.ld "$dir/programa.o" "$dir/runtime.o" -o "$dir/programa.elf"
Ejecutar-Zig ld.lld -T lenguaje/aplicacion.ld --oformat=binary "$dir/programa.o" "$dir/runtime.o" -o "$dir/programa.bin"
& $python lenguaje/empaquetar.py "$dir/programa.elf" "$dir/programa.bin" $Salida
if ($LASTEXITCODE -ne 0) { throw 'No se pudo empaquetar el ejecutable NXE.' }
