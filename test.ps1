# GUIA DE LECTURA NXK - test.ps1
# Construye una imagen de pruebas, arranca QEMU, verifica resultados y reconstruye la imagen normal.
# Contexto y orden de estudio: ver LEER-CODIGO.md en la carpeta principal.
param(
    [string]$FuenteApp = 'lenguaje/ejemplos/hola.nxf',
    [ValidateRange(16, 4096)][int]$MemoryMiB = 64,
    [switch]$MemoryMatrix,
    [ValidateSet('x86','x64')][string]$Architecture = 'x86'
)
$ErrorActionPreference = 'Stop'
$root = $PSScriptRoot
& "$root\build.ps1" -Test -FuenteApp $FuenteApp -Architecture $Architecture
$buildDir = if ($Architecture -eq 'x86') { "$root\build\x86" } else { "$root\build" }
$qemuName = if ($Architecture -eq 'x86') { 'qemu-system-i386.exe' } else { 'qemu-system-x86_64.exe' }
$cpu = if ($Architecture -eq 'x86') { 'qemu32' } else { 'qemu64' }
$modeMarker = if ($Architecture -eq 'x86') { 'NXK: PROTECTED MODE 32 OK' } else { 'NXK: LONG MODE OK' }
$qemu = "$root\tools\qemu\$qemuName"
$sizes = if ($MemoryMatrix) { @(32, 64, 128, 1024) } else { @($MemoryMiB) }
$results = @()
foreach ($size in $sizes) {
$out = "$buildDir\test-serial-$size-MiB.log"
$err = "$buildDir\test-qemu-$size-MiB.log"
$argsQemu = @('-machine','pc','-accel','tcg','-cpu',$cpu,'-m',($size.ToString()+'M'),'-drive',('file="'+$buildDir+'\nxk-test.img",format=raw,if=ide'),'-boot','c','-display','none','-serial','stdio','-monitor','none','-no-reboot','-device','isa-debug-exit,iobase=0xf4,iosize=0x04')
$info = New-Object System.Diagnostics.ProcessStartInfo
$info.FileName = $qemu
$info.Arguments = $argsQemu -join ' '
$info.WorkingDirectory = $root
$info.UseShellExecute = $false
$info.CreateNoWindow = $true
$info.RedirectStandardOutput = $true
$info.RedirectStandardError = $true
$process = New-Object System.Diagnostics.Process
$process.StartInfo = $info
$null = $process.Start()
$readOut = $process.StandardOutput.ReadToEndAsync()
$readErr = $process.StandardError.ReadToEndAsync()
if (!$process.WaitForExit(30000)) { $process.Kill(); throw "NXK no completo el arranque con $size MiB en 30 segundos. Consulta build\." }
$log = $readOut.GetAwaiter().GetResult()
[IO.File]::WriteAllText($out, $log)
[IO.File]::WriteAllText($err, $readErr.GetAwaiter().GetResult())
if ($process.ExitCode -ne 33 -or $log -notmatch 'NXK: BOOT PASS' -or !$log.Contains($modeMarker) -or !$log.Contains('NXK: ESPANOL OK') -or !$log.Contains('NXK: NXE OK')) { throw "Fallo de arranque con $size MiB: $log" }
if ($Architecture -eq 'x86' -and !$log.Contains('NXK: PE32 OK')) { throw "Fallo del puente Win32: $log" }
if (!$log.Contains('NXK: FORMATOS OK')) { throw "Fallo al rechazar ejecutables incorrectos: $log" }
if (!$log.Contains('NXK: LLENADO OK') -or !$log.Contains('NXK: COPIA OK')) { throw "Fallo de las rutinas de memoria: $log" }
if (!$log.Contains('NXK: MOVER OK')) { throw "Fallo en copia con solapamiento: $log" }
if (!$log.Contains('NXK: HEAP LIMITES OK')) { throw "Fallo en limites del heap: $log" }
if ($Architecture -eq 'x86') {
    if (!$log.Contains('NXK: PAGINACION Y WP OK')) { throw 'Falta inicio de paginacion.' }
    if (!$log.Contains('NXK: PAGINAS OK')) { throw "Fallo del gestor de paginas: $log" }
    if ($log -notmatch 'NXK: PAGINAS LIBRES: (\d+)' -or [int]$Matches[1] -lt (($size-10)*256)) { throw 'Cantidad de paginas inesperada.' }
    if ($log -notmatch 'NXK: RAM BIOS KiB: (\d+)') { throw 'Falta el mapa BIOS.' }
    $ramKiB=[int]$Matches[1]
    if ($ramKiB -lt ($size*1024-2048) -or $ramKiB -gt $size*1024) { throw "RAM BIOS inesperada: $ramKiB KiB para $size MiB" }
}
Write-Host "PRUEBA SUPERADA: NXK $Architecture, $size MiB de RAM virtual."
$results += [PSCustomObject]@{ architecture=$Architecture; memoryMiB=$size; cpu=$cpu; firmware='BIOS'; boot='PASS'; exitCode=$process.ExitCode }
$process.Dispose()
}
$results | ConvertTo-Json | Set-Content -Encoding UTF8 "$buildDir\memory-test-results.json"
Write-Host "Verificado: arranque $Architecture, ejecucion C y heap basico. No es una prueba de toda la RAM ni de hardware fisico."
& "$root\build.ps1" -FuenteApp $FuenteApp -Architecture $Architecture
