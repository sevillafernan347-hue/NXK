# Compila una aplicacion NEXTRY para la prueba CPL3 de una pagina.
param([string]$Fuente='lenguaje/ejemplos/usuario.nxf')
$ErrorActionPreference='Stop'
Set-Location $PSScriptRoot
$dir='build/x86/usuario-nxf'
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
