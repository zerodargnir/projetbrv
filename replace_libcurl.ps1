$log = "C:\Users\mewrd\Desktop\brv\replace_libcurl.log"
"=== $(Get-Date) ===" | Out-File $log
$dst = "C:\Program Files\WindowsApps\gumi.BraveFrontier_2.19.6.0_x86__9n4b6myyx21tw\libcurl.dll"
$src = "C:\Users\mewrd\Desktop\brv\offline-proxy\build\bin\libcurl.dll"
# backup hors du dossier protege
Copy-Item $dst "C:\Users\mewrd\Desktop\brv\libcurl_installed_backup.dll" -Force -ErrorAction SilentlyContinue
# prise de possession + droits D'ABORD
& takeown /f $dst 2>&1 | Out-File $log -Append
& icacls $dst /grant "*S-1-5-32-544:F" 2>&1 | Out-File $log -Append
# remplacement (ecrasement du fichier existant)
try {
    [System.IO.File]::Copy($src, $dst, $true)
    "COPY OK: $((Get-Item $dst).Length) octets" | Out-File $log -Append
} catch {
    "ECHEC COPY: $($_.Exception.Message)" | Out-File $log -Append
}
"=== FIN ===" | Out-File $log -Append
