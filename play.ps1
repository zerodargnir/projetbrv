# =====================================================================
#  play.ps1 - Lance Brave Frontier Offline en un clic.
#  Demarre le serveur emule (si besoin), lance le jeu, et arrete le
#  serveur quand tu fermes le jeu.
#
#  Usage:
#    .\play.ps1              # joue, puis arrete le serveur a la fermeture
#    .\play.ps1 -KeepServer  # laisse le serveur tourner apres le jeu
# =====================================================================
param(
    [switch]$KeepServer
)

$root      = $PSScriptRoot
$serverExe = Join-Path $root "server\build\standalone_frontend\Debug\gimuserverw.exe"
$deploy    = Join-Path $root "server\deploy"
$port      = 9960
$gameProc  = "BraveFrontier.Windows"

function Test-Port($p) {
    $c = New-Object System.Net.Sockets.TcpClient
    try { $c.Connect("127.0.0.1", $p); $c.Close(); return $true }
    catch { return $false }
    finally { $c.Dispose() }
}

# --- 1) Serveur -------------------------------------------------------
$startedServer = $false
if (Test-Port $port) {
    Write-Host "[=] Serveur deja en marche (port $port)." -ForegroundColor Green
}
else {
    if (-not (Test-Path $serverExe)) {
        Write-Host "[X] Serveur non compile: $serverExe" -ForegroundColor Red
        Write-Host "    Compile-le d'abord (voir CLAUDE.md / TESTS.md)." -ForegroundColor Red
        exit 1
    }
    Write-Host "[>] Demarrage du serveur..." -ForegroundColor Cyan
    Start-Process -FilePath $serverExe -WorkingDirectory $deploy -WindowStyle Minimized | Out-Null
    $startedServer = $true
    $deadline = (Get-Date).AddSeconds(20)
    while (-not (Test-Port $port) -and (Get-Date) -lt $deadline) { Start-Sleep -Milliseconds 300 }
    if (Test-Port $port) {
        Write-Host "[=] Serveur pret sur 127.0.0.1:$port" -ForegroundColor Green
    }
    else {
        Write-Host "[X] Le serveur n'a pas demarre a temps." -ForegroundColor Red
        exit 1
    }
}

# --- 2) Client installe ? --------------------------------------------
$pkg = Get-AppxPackage *BraveFrontier* -ErrorAction SilentlyContinue
if (-not $pkg) {
    Write-Host "[X] Client Brave Frontier non installe (Add-AppxPackage)." -ForegroundColor Red
    exit 1
}

# --- 3) Lancer le jeu -------------------------------------------------
$appid = ($pkg | Get-AppxPackageManifest).Package.Applications.Application.Id
$aumid = "$($pkg.PackageFamilyName)!$appid"
Write-Host "[>] Lancement du jeu..." -ForegroundColor Cyan
Start-Process "shell:AppsFolder\$aumid"

# --- 4) Attendre la fermeture du jeu, puis nettoyer ------------------
if ($KeepServer) {
    Write-Host "[=] Jeu lance. Serveur laisse en marche (-KeepServer)." -ForegroundColor Green
    exit 0
}

if (-not $startedServer) {
    Write-Host "[=] Jeu lance. (serveur deja la avant ce script -> non arrete)" -ForegroundColor Green
    exit 0
}

Write-Host "[i] Ferme le jeu pour arreter le serveur automatiquement..." -ForegroundColor Yellow
Start-Sleep -Seconds 5  # laisser le process du jeu apparaitre
while (Get-Process $gameProc -ErrorAction SilentlyContinue) { Start-Sleep -Seconds 2 }

Write-Host "[>] Jeu ferme. Arret du serveur..." -ForegroundColor Cyan
Get-Process gimuserverw -ErrorAction SilentlyContinue | Stop-Process -Force
Write-Host "[=] A bientot !" -ForegroundColor Green
