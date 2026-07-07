$ErrorActionPreference = 'Continue'
$log = "C:\Users\mewrd\Desktop\brv\recover_client.log"
"=== $(Get-Date) ===" | Out-File $log
$dir = "C:\Program Files\WindowsApps\gumi.BraveFrontier_2.19.6.0_x86__9n4b6myyx21tw"
$manifest = Join-Path $dir "AppxManifest.xml"
$appx = "C:\Users\mewrd\Desktop\brv\client-appx\BraveFrontierPatched.appx"
$srcLib = "C:\Users\mewrd\Desktop\brv\offline-proxy\build\bin\libcurl.dll"

# 0. DEBLOQUER le sideloading force OFF par la GPO (cause du 0x80073CFF)
$gp = "HKLM:\SOFTWARE\Policies\Microsoft\Windows\Appx"
try {
    if (-not (Test-Path $gp)) { New-Item -Path $gp -Force | Out-Null }
    Set-ItemProperty -Path $gp -Name AllowAllTrustedApps -Value 1 -Type DWord
    Set-ItemProperty -Path $gp -Name AllowDevelopmentWithoutDevLicense -Value 1 -Type DWord
    "GPO Appx: AllowAllTrustedApps=1 pose" | Out-File $log -Append
} catch { "GPO set echoue: $($_.Exception.Message)" | Out-File $log -Append }
# aussi la cle AppModelUnlock (deja 1 mais on assure)
$k = "HKLM:\SOFTWARE\Microsoft\Windows\CurrentVersion\AppModelUnlock"
try { if(-not(Test-Path $k)){New-Item $k -Force|Out-Null}; Set-ItemProperty $k -Name AllowAllTrustedApps -Value 1 -Type DWord } catch {}

function Repatch-Libcurl { param($installDir)
    $dstLib = Join-Path $installDir "libcurl.dll"
    & takeown /f $dstLib 2>&1 | Out-File $log -Append
    & icacls $dstLib /grant "*S-1-5-32-544:F" 2>&1 | Out-File $log -Append
    try { [System.IO.File]::Copy($srcLib, $dstLib, $true); "LIBCURL REPATCH OK: $((Get-Item $dstLib).Length) o" | Out-File $log -Append }
    catch { "LIBCURL REPATCH ECHEC: $($_.Exception.Message)" | Out-File $log -Append }
}

$ok = $false
# Tentative 1 : register l'install intacte (garde le patch)
if (Test-Path $manifest) {
    try { Add-AppxPackage -Register $manifest -DisableDevelopmentMode -ErrorAction Stop
        "REGISTER OK (patch conserve)" | Out-File $log -Append; $ok = $true
    } catch { "register echoue: $($_.Exception.Message)" | Out-File $log -Append }
}
# Tentative 2 : reinstall appx + repatch
if (-not $ok) {
    "--- fallback reinstall ---" | Out-File $log -Append
    Get-AppxPackage -AllUsers *BraveFrontier* | Remove-AppxPackage -AllUsers -ErrorAction SilentlyContinue
    Start-Sleep -Seconds 2
    try { Add-AppxPackage -Path $appx -ErrorAction Stop; "REINSTALL OK" | Out-File $log -Append
        $nd = (Get-AppxPackage *BraveFrontier*).InstallLocation; if ($nd) { Repatch-Libcurl $nd }; $ok = $true
    } catch { "reinstall echoue: $($_.Exception.Message)" | Out-File $log -Append }
}

Start-Sleep -Seconds 2
$pkg = Get-AppxPackage *BraveFrontier*
if ($pkg) {
    "FINAL: PRESENT $($pkg.PackageFullName)" | Out-File $log -Append
    $lc = Join-Path $pkg.InstallLocation "libcurl.dll"
    if (Test-Path $lc) { "FINAL libcurl SHA256: $((Get-FileHash $lc -Algorithm SHA256).Hash)" | Out-File $log -Append }
} else { "FINAL: ABSENT (echec)" | Out-File $log -Append }
"=== FIN ===" | Out-File $log -Append
