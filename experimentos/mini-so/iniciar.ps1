# Abre la interfaz del kernel real en QEMU. Solo discos virtuales snapshot.
param([switch]$Recompilar)
$ErrorActionPreference='Stop'
$raizNXK=Split-Path (Split-Path $PSScriptRoot -Parent) -Parent
Set-Location $raizNXK
if ($Recompilar -or !(Test-Path 'build/x86-interfaz/nxk.img')) {
    & .\build.ps1 -Architecture x86 -Interfaz
}
$nombreNXK='mini-'+[Guid]::NewGuid().ToString('N')+'.img'
$imagenNXK=Join-Path 'build/x86-interfaz' $nombreNXK
Copy-Item -LiteralPath 'build/x86-interfaz/nxk.img' -Destination $imagenNXK
try {
    & .\tools\qemu\qemu-system-i386.exe -name 'MINI NXK - experimento grafico' -machine pc -accel tcg -cpu qemu32 -m 64M -drive "file=$imagenNXK,format=raw,if=ide,index=0,snapshot=on" -boot c -display gtk,zoom-to-fit=on -serial "file:build/x86-interfaz/$nombreNXK.log" -monitor none -no-reboot
} finally {
    if (Test-Path -LiteralPath $imagenNXK) { Remove-Item -LiteralPath $imagenNXK }
}
