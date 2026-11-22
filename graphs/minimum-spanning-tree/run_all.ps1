# Builds and tests the C (C11 and C17) and C++17 programs with the Visual Studio
# (MSVC) toolchain, runs the timing benchmark, and prints a Markdown report.
# Builds go to %TEMP%, so nothing is left in the repository.
# Run from a "Developer PowerShell for VS" prompt, or any shell where cmake finds MSVC:
#   .\run_all.ps1 > results-windows.md
Set-Location $PSScriptRoot
$os = (Get-CimInstance Win32_OperatingSystem).Caption
$rows = @()

foreach ($std in 11, 17) {
    $build = Join-Path $env:TEMP "mst-msvc-c$std"
    if (Test-Path $build) { Remove-Item -Recurse -Force $build }

    cmake -S . -B $build "-DCMAKE_C_STANDARD=$std" | Out-Null
    $built = $false
    if ($LASTEXITCODE -eq 0) {
        $log = cmake --build $build --config Release 2>&1 | Out-String
        $built = ($LASTEXITCODE -eq 0)
        if (-not $built) {
            # Keep the compiler's messages so the failure can be diagnosed.
            $logFile = Join-Path $PSScriptRoot "msvc-build-c$std.log"
            $log | Out-File -Encoding utf8 $logFile
        }
    }
    $rows += "| MSVC /W4 /WX, C$std + C++17, Release build | $(if ($built) { 'built' } else { "BUILD FAILED (see msvc-build-c$std.log)" }) |"

    if ($built) {
        $out = ctest --test-dir $build -C Release --output-on-failure 2>&1 | Out-String
        if ($out -match "tests failed out of") {
            if ($out -notmatch " 0 tests failed") {
                # Keep the failing test's output so it can be diagnosed.
                $out | Out-File -Encoding utf8 (Join-Path $PSScriptRoot "msvc-test-c$std.log")
            }
        }
        $line = ($out -split "`n" | Where-Object { $_ -match "tests passed|tests failed" } | Select-Object -First 1).Trim()
        $rows += "| MSVC, C$std, ctest (brute force, Prim, C and C++ outputs) | $line |"
    }
}

Write-Output ""
Write-Output "Environment: $os, $((cmake --version | Select-Object -First 1)), $([Environment]::ProcessorCount) logical CPUs"
Write-Output ""
Write-Output "| Check | Result |"
Write-Output "| --- | --- |"
$rows | ForEach-Object { Write-Output $_ }

$timing = Join-Path $env:TEMP "mst-msvc-c11\Release\mst_speed.exe"
if (Test-Path $timing) {
    Write-Output ""
    Write-Output "Timing (MSVC, Release build, /O2):"
    Write-Output ""
    Write-Output '```'
    & $timing
    Write-Output '```'
}
