# Lance toute la suite de tests Brave Frontier Offline.
# Usage:  .\run_tests.ps1
# Note: unittest ecrit son rapport sur stderr ; on ne met PAS ErrorActionPreference=Stop
# pour ne pas traiter ces lignes comme des erreurs. Le code retour vient de python.
$py = "C:\Users\mewrd\AppData\Local\Programs\Python\Python314"
$env:Path = "$py;" + $env:Path
Set-Location $PSScriptRoot
Write-Host "=== Suite de tests Brave Frontier Offline ===" -ForegroundColor Cyan
python -m unittest discover -p "test_*.py" -v
