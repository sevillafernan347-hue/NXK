# Compila una aplicacion NEXTRY para la prueba CPL3 de una pagina.
param([string]$Fuente='lenguaje/ejemplos/usuario.nxf',[string]$Salida='')
$ErrorActionPreference='Stop'
Set-Location $PSScriptRoot
$dir='build/x86/usuario-nxf'
if ($Salida) {
    $carpeta=Split-Path $Salida -Parent
    if (!$carpeta) { $carpeta='.' }
    $dir=Join-Path $carpeta (([IO.Path]::GetFileNameWithoutExtension($Salida))+'-usuario-intermedios')
}
New-Item -ItemType Directory -Force $dir | Out-Null
$zig=Join-Path $PSScriptRoot 'tools/zig/zig.exe'
$python=Join-Path $PSScriptRoot 'tools/python/python.exe'
& $python lenguaje/compilador.py $Fuente -o "$dir/programa.c"
if ($LASTEXITCODE -ne 0) { throw 'Fuente de usuario invalida.' }
function Z { & $zig @args; if ($LASTEXITCODE -ne 0) { throw 'Error compilando aplicacion usuario.' } }
$flags=@('-target','x86-freestanding-none','-ffreestanding','-fno-stack-protector','-fno-pic','-fno-builtin','-mno-sse','-mno-sse2','-mno-mmx','-Os','-Wall','-Wextra','-Werror','-Ilib')
Z cc @flags -c "$dir/programa.c" -o "$dir/programa.o"
Z cc @flags -c lenguaje/runtime_usuario.c -o "$dir/runtime.o"
Z ld.lld -T lenguaje/usuario.ld "$dir/runtime.o" "$dir/programa.o" -o "$dir/programa.elf"
Z ld.lld -T lenguaje/usuario.ld --oformat=binary "$dir/runtime.o" "$dir/programa.o" -o "$dir/programa.bin"

if (!$Salida) { $Salida="$dir/programa.nxe" }
$padre=Split-Path $Salida -Parent
if ($padre) { New-Item -ItemType Directory -Force $padre | Out-Null }
& $python lenguaje/empaquetar_usuario.py "$dir/programa.elf" "$dir/programa.bin" $Salida
if ($LASTEXITCODE -ne 0) { throw 'No se pudo crear el NXE2 de usuario.' }
