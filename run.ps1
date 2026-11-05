# GUIA DE LECTURA NXK - run.ps1
# Arranca QEMU con una copia de la imagen para poder reconstruir mientras la maquina virtual esta abierta.
# Contexto y orden de estudio: ver LEER-CODIGO.md en la carpeta principal.
param(
    [string]$Volumen = '',
    [string]$Persistencia = '',
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
$extra=@()
if ($Volumen -and $Persistencia) { throw "Elegir Volumen NXFS o Persistencia NXPD1, no ambos." }
if ($Volumen) {
    if ($Architecture -ne 'x86') { throw 'Volumen ATA disponible solo en x86.' }
    $rutaVolumen=(Resolve-Path -LiteralPath $Volumen).Path
    $extra=@('-drive',"file=$rutaVolumen,format=raw,if=ide,index=1,snapshot=on")
}
if ($Persistencia) {
    if ($Architecture -ne 'x86') { throw 'Persistencia disponible solo en x86.' }
    $archivoDatos=Get-Item -LiteralPath $Persistencia
    if ($archivoDatos.PSIsContainer -or $archivoDatos.Extension -ne '.img' -or $archivoDatos.Length -ne 9728) { throw 'Se requiere un archivo NXPD1 .img de 9728 bytes.' }
    $rutaDatos=$archivoDatos.FullName
    if ($rutaDatos.Contains(',')) { throw 'La ruta del disco no debe contener comas.' }
    $cabeceraDatos=[System.IO.File]::ReadAllBytes($rutaDatos)
    if ([System.Text.Encoding]::ASCII.GetString($cabeceraDatos,0,8) -ne 'NXPD0001') { throw 'Firma NXPD1 incorrecta.' }
    $extra=@('-drive',"file=$rutaDatos,format=raw,if=ide,index=1,cache=writeback")
}
$sessionName = 'nxk-session-' + [Guid]::NewGuid().ToString('N') + '.img'
$sessionPath = Join-Path $PSScriptRoot "$buildDir/$sessionName"
Copy-Item -LiteralPath (Join-Path $PSScriptRoot "$buildDir/nxk.img") -Destination $sessionPath
try {
    & $qemu -name "NXK $Architecture - Computadora virtual - $MemoryMiB MiB" -machine pc -accel tcg -cpu $cpu -m ($MemoryMiB.ToString()+'M') -drive "file=$buildDir/$sessionName,format=raw,if=ide,index=0,snapshot=on" @extra -boot c -display gtk -serial "file:$buildDir/serial-$sessionName.log" -monitor none -no-reboot
} finally {
    if (Test-Path -LiteralPath $sessionPath) { Remove-Item -LiteralPath $sessionPath }
}
