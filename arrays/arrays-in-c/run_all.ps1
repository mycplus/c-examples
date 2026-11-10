# Builds and tests with the Visual Studio (MSVC) toolchain in C11 and C17 and prints a Markdown table.
# Run from a "Developer PowerShell for VS" prompt, or any shell where cmake finds MSVC.
Set-Location $PSScriptRoot
$os = (Get-CimInstance Win32_OperatingSystem).Caption
$rows = @()

foreach ($std in 11, 17) {
    $build = Join-Path $env:TEMP "arrays-in-c-msvc-c$std"
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
            Write-Output "Build log saved to $logFile"
        }
    }
    $rows += "| MSVC /W4 /WX, C$std, Release build | $(if ($built) { 'built' } else { 'BUILD FAILED' }) |"

    if ($built) {
        $out = ctest --test-dir $build -C Release 2>&1 | Out-String
        $line = ($out -split "`n" | Where-Object { $_ -match "tests passed|tests failed" } | Select-Object -First 1).Trim()
        $rows += "| MSVC, C$std, ctest (unit tests and byte-for-byte output checks) | $line |"
    }
}

Write-Output ""
Write-Output "Environment: $os, $((cmake --version | Select-Object -First 1))"
Write-Output ""
Write-Output "| Check | Result |"
Write-Output "| --- | --- |"
$rows | ForEach-Object { Write-Output $_ }
