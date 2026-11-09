# Builds and tests with the Visual Studio (MSVC) toolchain and prints a Markdown table.
# Run from a "Developer PowerShell for VS" prompt, or any shell where cmake finds MSVC.
Set-Location $PSScriptRoot
$os = (Get-CimInstance Win32_OperatingSystem).Caption
$build = Join-Path $env:TEMP "pointers-msvc-build"
if (Test-Path $build) { Remove-Item -Recurse -Force $build }
$rows = @()

cmake -S . -B $build | Out-Null
$configured = ($LASTEXITCODE -eq 0)
if ($configured) {
    cmake --build $build --config Release | Out-Null
    $built = ($LASTEXITCODE -eq 0)
} else { $built = $false }
$rows += "| MSVC /W4 /WX, C11, Release build | $(if ($built) { 'built' } else { 'BUILD FAILED' }) |"

if ($built) {
    $out = ctest --test-dir $build -C Release 2>&1 | Out-String
    $line = ($out -split "`n" | Where-Object { $_ -match "tests passed|tests failed" } | Select-Object -First 1).Trim()
    $rows += "| ctest (unit tests and byte-for-byte output checks) | $line |"
}

Write-Output ""
Write-Output "Environment: $os, $((cmake --version | Select-Object -First 1))"
Write-Output ""
Write-Output "| Check | Result |"
Write-Output "| --- | --- |"
$rows | ForEach-Object { Write-Output $_ }
