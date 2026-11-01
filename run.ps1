# GUIA DE LECTURA NXK - run.ps1
# Arranca QEMU con una copia de la imagen para poder reconstruir mientras la maquina virtual esta abierta.
# Contexto y orden de estudio: ver LEER-CODIGO.md en la carpeta principal.
param(
    [ValidateRange(16, 4096)][int]$MemoryMiB = 64,
    [ValidateSet('x86','x64')][string]$Architecture = 'x86'
)
$ErrorActionPreference = 'Stop'
Set-Location $PSScriptRoot
$buildDir = if ($Architecture -eq 'x86') { 'build/x86' } else { 'build' }
$qemuName = if ($Architecture -eq 'x86') { 'qemu-system-i386.exe' } else { 'qemu-system-x86_64.exe' }
$cpu = if ($Architecture -eq 'x86') { 'qemu32' } else { 'qemu64' }
$qemu = Join-Path $PSScriptRoot "tools/qemu/$qemuName"
if (!(Test-Path "$buildDir/nxk.img")) { & .\build.ps1 -Architecture $Architecture }
Write-Host 'Haz clic en la ventana NXK para escribir: help, version, mem, halt. Ctrl+Alt+G libera el teclado y el raton.'
$sessionName = 'nxk-session-' + [Guid]::NewGuid().ToString('N') + '.img'
$sessionPath = Join-Path $PSScriptRoot "$buildDir/$sessionName"
Copy-Item -LiteralPath (Join-Path $PSScriptRoot "$buildDir/nxk.img") -Destination $sessionPath
try {
    & $qemu -name "NXK $Architecture - Computadora virtual - $MemoryMiB MiB" -machine pc -accel tcg -cpu $cpu -m ($MemoryMiB.ToString()+'M') -drive "file=$buildDir/$sessionName,format=raw,if=ide" -snapshot -boot c -display gtk -serial "file:$buildDir/serial-$sessionName.log" -monitor none -no-reboot
} finally {
    if (Test-Path -LiteralPath $sessionPath) { Remove-Item -LiteralPath $sessionPath }
}
