# Builds and tests everything with the Visual Studio (MSVC) toolchain in C11
# and C17, then lists the test fixture with the Windows build, and prints a
# Markdown report. Run from a "Developer PowerShell for VS":
#   .\run_all.ps1 > results-windows.md
Set-Location $PSScriptRoot
$os = (Get-CimInstance Win32_OperatingSystem).Caption
$rows = @()

foreach ($std in 11, 17) {
    $build = Join-Path $env:TEMP "dirlist-msvc-c$std"
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
    $rows += "| MSVC /W4 /WX, C$std, Release build | $(if ($built) { 'built' } else { "BUILD FAILED (see msvc-build-c$std.log)" }) |"

    if ($built) {
        $out = ctest --test-dir $build -C Release 2>&1 | Out-String
        $line = ($out -split "`n" | Where-Object { $_ -match "tests passed|tests failed" } | Select-Object -First 1).Trim()
        $failed = ($out -split "`n" | Where-Object { $_ -match "\*\*\*Failed|Not Run" }) -join "; "
        $rows += "| MSVC, C$std, ctest (unit tests and byte-for-byte outputs) | $line $failed |"
    }
}

Write-Output ""
Write-Output "Environment: $os, $((cmake --version | Select-Object -First 1))"
Write-Output ""
Write-Output "| Check | Result |"
Write-Output "| --- | --- |"
$rows | ForEach-Object { Write-Output $_ }

# The listing as Windows sees it, for the article (times in UTC).
$bin = Join-Path $env:TEMP "dirlist-msvc-c11\Release"
if (Test-Path (Join-Path $bin "dirlist.exe")) {
    Push-Location $env:TEMP
    if (Test-Path "dirlist-fixture") { Remove-Item -Recurse -Force "dirlist-fixture" }
    & (Join-Path $bin "make_fixture.exe") dirlist-fixture
    # Let cmd.exe redirect the raw bytes to a file, then read them as UTF-8:
    # PowerShell's own pipeline would re-decode them with the console code page.
    # Relative paths (we are in %TEMP%) avoid quoting problems with spaces.
    cmd /c "dirlist-msvc-c11\Release\dirlist.exe -a -u -s dirlist-fixture > dirlist-listing.txt"
    $listing = Join-Path $env:TEMP "dirlist-listing.txt"
    Write-Output ""
    Write-Output '```'
    Get-Content -Encoding UTF8 $listing | ForEach-Object { Write-Output $_ }
    Write-Output '```'
    Pop-Location
}
