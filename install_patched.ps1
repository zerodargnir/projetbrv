$log = "C:\Users\mewrd\Desktop\brv\install_patched.log"
"=== $(Get-Date) ===" | Out-File $log

# 1) Lever la policy de groupe qui bloque le sideloading
try {
    $pk = "HKLM:\SOFTWARE\Policies\Microsoft\Windows\Appx"
    if (-not (Test-Path $pk)) { New-Item -Path $pk -Force | Out-Null }
    Set-ItemProperty -Path $pk -Name AllowAllTrustedApps -Type DWord -Value 1
    Set-ItemProperty -Path $pk -Name AllowDevelopmentWithoutDevLicense -Type DWord -Value 1
    "Policy Appx: AllowAllTrustedApps=1, AllowDevelopmentWithoutDevLicense=1" | Out-File $log -Append
} catch { "Policy: $($_.Exception.Message)" | Out-File $log -Append }

# 2) Cert de confiance (machine)
try {
    Import-Certificate -FilePath "C:\Users\mewrd\Desktop\brv\client-appx\MyKey.cer" -CertStoreLocation Cert:\LocalMachine\TrustedPeople -ErrorAction Stop | Out-Null
    Import-Certificate -FilePath "C:\Users\mewrd\Desktop\brv\client-appx\MyKey.cer" -CertStoreLocation Cert:\LocalMachine\Root -ErrorAction Stop | Out-Null
    "Cert importe (TrustedPeople + Root)" | Out-File $log -Append
} catch { "Import cert: $($_.Exception.Message)" | Out-File $log -Append }

# 3) Installer le package patche
try {
    $pkg = Get-AppxPackage *BraveFrontier*
    if ($pkg) { Remove-AppxPackage $pkg.PackageFullName -ErrorAction SilentlyContinue; "Ancien package retire" | Out-File $log -Append }
    Add-AppxPackage -Path "C:\Users\mewrd\Desktop\brv\client-appx\BraveFrontierPatched.appx" -ErrorAction Stop
    "INSTALL OK" | Out-File $log -Append
    (Get-AppxPackage *BraveFrontier*) | Select-Object Name,Version | Format-List | Out-String | Out-File $log -Append
} catch { "INSTALL ECHEC: $($_.Exception.Message)" | Out-File $log -Append }
"=== FIN ===" | Out-File $log -Append
