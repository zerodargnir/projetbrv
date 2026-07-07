# ============================================================
#  Brave Frontier Offline - Lancer le jeu AVEC les bannieres
# ============================================================
# - Demarre le serveur gimuserverw
# - Lance le client APPX
# - Attache cdb et applique le patch a chaud (3 NOP) qui fait
#   apparaitre les bannieres d'invocation (bypass filtre date/items)
#
# NB IMPORTANT : le patch est applique EN MEMOIRE (cdb), PAS grave
# dans le .exe. Ne JAMAIS reinstaller un .exe patche : ca vide le
# cache de donnees du client et relance toute la 1ere synchro.
# ============================================================

$ErrorActionPreference = "SilentlyContinue"
$root   = "C:\Users\mewrd\Desktop\brv"
$srvExe = "$root\server\build\standalone_frontend\Debug\gimuserverw.exe"
$cdb    = "C:\Program Files (x86)\Windows Kits\10\Debuggers\x86\cdb.exe"

# 1) Serveur
if (-not (Get-Process gimuserverw -EA SilentlyContinue)) {
    Start-Process -FilePath $srvExe -WorkingDirectory "$root\server\deploy" -WindowStyle Minimized `
        -RedirectStandardOutput "$root\server\deploy\server-console.log" `
        -RedirectStandardError  "$root\server\deploy\server-console.err.log"
    Start-Sleep -Seconds 3
    Write-Host "[+] Serveur demarre" -ForegroundColor Green
} else { Write-Host "[=] Serveur deja lance" -ForegroundColor Yellow }

# 2) Client
$pkg = Get-AppxPackage *BraveFrontier*
$appid = ($pkg | Get-AppxPackageManifest).Package.Applications.Application.Id
Start-Process "shell:AppsFolder\$($pkg.PackageFamilyName)!$appid"
Write-Host "[+] Client lance, attente du boot..." -ForegroundColor Green
Start-Sleep -Seconds 18

# 3) Patch a chaud des bannieres (cdb)
$p = (Get-Process BraveFrontier.Windows -EA SilentlyContinue).Id
if ($p) {
    Start-Process -FilePath $cdb -ArgumentList "-p",$p,"-cf","$root\cdb_patch.txt","-logo","$root\cdb_patch_run.txt" -WindowStyle Minimized
    Start-Sleep -Seconds 4
    Write-Host "[+] Patch bannieres applique (cdb attache)" -ForegroundColor Green
    Write-Host "    -> Va dans Summon : les bannieres s'affichent." -ForegroundColor Cyan
} else {
    Write-Host "[!] Client introuvable - relance le script" -ForegroundColor Red
}
