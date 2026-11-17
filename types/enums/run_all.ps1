# Builds and tests the C (C11 and C17) and C++17 programs with the Visual Studio
# (MSVC) toolchain and prints a Markdown report, including what MSVC chose as
# the underlying type of an enum. Run from a "Developer PowerShell for VS":
#   .\run_all.ps1 > results-windows.md
Set-Location $PSScriptRoot
$os = (Get-CimInstance Win32_OperatingSystem).Caption
$rows = @()

foreach ($std in 11, 17) {
    $build = Join-Path $env:TEMP "enums-msvc-c$std"
    if (Test-Path $build) { Remove-Item -Recurse -Force $build }

    cmake -S . -B $build "-DCMAKE_C_STANDARD=$std" | Out-Null
    $built = $false
    if ($LASTEXITCODE -eq 0) {
        $log = cmake --build $build --config Release 2>&1 | Out-String
        $built = ($LASTEXITCODE -eq 0)
        if (-not $built) {
            $logFile = Join-Path $PSScriptRoot "msvc-build-c$std.log"
            $log | Out-File -Encoding utf8 $logFile
        }
    }
    $rows += "| MSVC /W4 /WX, C$std + C++17, Release build | $(if ($built) { 'built' } else { "BUILD FAILED (see msvc-build-c$std.log)" }) |"

    if ($built) {
        $out = ctest --test-dir $build -C Release 2>&1 | Out-String
        $line = ($out -split "`n" | Where-Object { $_ -match "tests passed|tests failed" } | Select-Object -First 1).Trim()
        $failed = ($out -split "`n" | Where-Object { $_ -match "\*\*\*Failed|Not Run" }) -join "; "
        $rows += "| MSVC, C$std, ctest (unit tests, outputs, 5 programs that must not compile) | $line $failed |"
    }
}

$underlying = Join-Path $env:TEMP "enums-msvc-c11\Release\underlying.exe"
if (Test-Path $underlying) {
    $rows += "| underlying, MSVC | $((& $underlying) -join '; ') |"
}

Write-Output ""
Write-Output "Environment: $os, $((cmake --version | Select-Object -First 1))"
Write-Output ""
Write-Output "| Check | Result |"
Write-Output "| --- | --- |"
$rows | ForEach-Object { Write-Output $_ }
