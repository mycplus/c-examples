# run_all.ps1 - the Windows checks behind the article's "What I Verified"
# table: an MSVC build with /W4 /WX and every CTest test.
# Run from this directory in a Developer PowerShell:  .\run_all.ps1
# Writes the result lines to results-windows.md; if the build fails, the
# compiler output is in msvc-build.log.
Set-Location $PSScriptRoot
$results = @()
function Row($name, $ok) { $script:results += ("{0,-48} {1}" -f $name, $(if ($ok) { "PASS" } else { "FAIL" })) }

if (Test-Path build-msvc) { Remove-Item -Recurse -Force build-msvc }
cmake -S . -B build-msvc *> msvc-build.log
$built = $LASTEXITCODE -eq 0
if ($built) { cmake --build build-msvc --config Release *>> msvc-build.log; $built = $LASTEXITCODE -eq 0 }
Row "MSVC build (/W4 /WX)" $built
if ($built) {
    Remove-Item msvc-build.log
    ctest --test-dir build-msvc -C Release --output-on-failure
    Row "MSVC CTest (unit, brute force, outputs)" ($LASTEXITCODE -eq 0)
}

$report = @("Environment: " + [System.Environment]::OSVersion.VersionString)
$report += (Get-Command cl.exe -ErrorAction SilentlyContinue | ForEach-Object { "cl.exe " + $_.Version })
$report += $results
$report | Set-Content results-windows.md
$report
