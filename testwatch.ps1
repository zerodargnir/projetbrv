# Analyse les commandes serveur recues depuis N secondes (defaut 25).
# Usage: .\testwatch.ps1 [secondes]
param([int]$Since = 25)

$log  = "C:\Users\mewrd\Desktop\brv\server\deploy\log"
$cut  = (Get-Date).AddSeconds(-$Since)

# Connu : ID -> nom lisible
$names = @{
  "MfZyu1q9"="Initialize"; "cTZ3W2JG"="UserInfo"; "2o4axPIC"="FriendGet";
  "uYF93Mhc"="ControlCenterEnter"; "nJ3A7qFp"="BadgeInfo"; "Zw3WIoWu"="ChallengeArenaResetInfo";
  "m2Ve9PkJ"="DeckEdit"; "Uo86DcRh"="GatchaList"; "NiYWKdzs"="HomeInfo(STUB)";
  "jE6Sp0q4"="MissionStart"; "ynB7X5P9"="UpdateInfoLight(STUB)"
}

$gameAlive = [bool](Get-Process BraveFrontier.Windows -ErrorAction SilentlyContinue)
Write-Host ("=== Jeu: " + $(if($gameAlive){"VIVANT"}else{"FERME (crash?)"}) + " | commandes des dernieres ${Since}s ===") -ForegroundColor Cyan

$files = Get-ChildItem "$log\http_log_*.log" -ErrorAction SilentlyContinue |
         Where-Object { $_.LastWriteTime -ge $cut -and $_.Name -notlike "*webpage*" }

if (-not $files) { Write-Host "(aucune commande d'action recente)" -ForegroundColor DarkGray; return }

foreach ($f in ($files | Sort-Object LastWriteTime)) {
  $cmd = ($f.Name -replace 'http_log_(.+)_\d{8}_\d{6}\.log','$1')
  $nm  = $names[$cmd]; if (-not $nm) { $nm = "$cmd <-- NON ENREGISTRE (absent)" }
  $txt = (Get-Content $f.FullName -Raw -ErrorAction SilentlyContinue) -replace "`0",""
  $resp = ""
  if ($txt -match 'RESPONSE:\s*(.*)') { $resp = $matches[1].Trim() }
  $empty = ($resp -eq "{}" -or $resp -eq "")
  $tag = if ($empty) { "[VIDE {} -> probleme]" } else { "[OK donnees]" }
  $color = if ($empty) { "Red" } else { "Green" }
  Write-Host ("  {0,-28} {1}" -f $nm, $tag) -ForegroundColor $color
}

# 404 assets
$dlc = "$log\dlc_404.log"
if ((Test-Path $dlc) -and (Get-Item $dlc).Length -gt 0) {
  Write-Host "  [!] des assets 404 (voir dlc_404.log)" -ForegroundColor Yellow
}
