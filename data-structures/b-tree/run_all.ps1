# run_all.ps1 - the Windows checks behind the article's "What I Verified"
# table: an MSVC build with /W4 /WX and every CTest test.
# Run from this directory in a Developer PowerShell:  .\run_all.ps1
$ErrorActionPreference = "Stop"
Set-Location $PSScriptRoot
$results = @()
function Row($name, $ok) { $script:results += ("{0,-44} {1}" -f $name, $(if ($ok) { "PASS" } else { "FAIL" })) }

if (Test-Path build-msvc) { Remove-Item -Recurse -Force build-msvc }
cmake -S . -B build-msvc
$built = $LASTEXITCODE -eq 0
if ($built) { cmake --build build-msvc --config Release; $built = $LASTEXITCODE -eq 0 }
Row "MSVC build (/W4 /WX)" $built
if ($built) {
    ctest --test-dir build-msvc -C Release --output-on-failure
    Row "MSVC CTest (unit, model-based, outputs)" ($LASTEXITCODE -eq 0)
}

""
"Environment: " + [System.Environment]::OSVersion.VersionString
$results
