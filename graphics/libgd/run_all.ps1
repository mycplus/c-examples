# Builds and tests everything with the Visual Studio (MSVC) toolchain in C11
# and C17, using LibGD from vcpkg, and prints a Markdown report.
#
# One-time setup (a few minutes; builds LibGD with PNG, JPEG and FreeType):
#   git clone https://github.com/microsoft/vcpkg C:\vcpkg
#   C:\vcpkg\bootstrap-vcpkg.bat
#   C:\vcpkg\vcpkg install "libgd[core,png,jpeg,freetype]:x64-windows"
# Then, from a "Developer PowerShell for VS":
#   .\run_all.ps1 > results-windows.md
# Set $env:LIBGD_VCPKG first if vcpkg is not in C:\vcpkg.
Set-Location $PSScriptRoot
$os = (Get-CimInstance Win32_OperatingSystem).Caption
# Visual Studio's Developer PowerShell sets VCPKG_ROOT to the copy of vcpkg
# bundled with Visual Studio, which only works in manifest mode. Use the first
# vcpkg that actually has LibGD installed: $env:LIBGD_VCPKG, C:\vcpkg, then VCPKG_ROOT.
$candidates = @($env:LIBGD_VCPKG, "C:\vcpkg", $env:VCPKG_ROOT) | Where-Object { $_ }
$vcpkg = $candidates | Where-Object {
    Test-Path (Join-Path $_ "installed\x64-windows\include\gd.h")
} | Select-Object -First 1
if (-not $vcpkg) {
    Write-Output "LibGD was not found in any vcpkg ($($candidates -join ', ')). Run:"
    Write-Output "  C:\vcpkg\vcpkg install `"libgd[core,png,jpeg,freetype]:x64-windows`""
    Write-Output "and check its last lines for errors, or set LIBGD_VCPKG to your vcpkg folder."
    exit 1
}
$toolchain = Join-Path $vcpkg "scripts\buildsystems\vcpkg.cmake"
$rows = @()

foreach ($std in 11, 17) {
    $build = Join-Path $env:TEMP "libgd-msvc-c$std"
    if (Test-Path $build) { Remove-Item -Recurse -Force $build }

    $cfg = cmake -S . -B $build "-DCMAKE_C_STANDARD=$std" "-DCMAKE_TOOLCHAIN_FILE=$toolchain" `
                 "-DVCPKG_TARGET_TRIPLET=x64-windows" 2>&1 | Out-String
    $built = $false
    if ($LASTEXITCODE -eq 0) {
        $log = cmake --build $build --config Release 2>&1 | Out-String
        $built = ($LASTEXITCODE -eq 0)
        if (-not $built) { $log | Out-File -Encoding utf8 (Join-Path $PSScriptRoot "msvc-build-c$std.log") }
    } else {
        $cfg | Out-File -Encoding utf8 (Join-Path $PSScriptRoot "msvc-configure-c$std.log")
    }
    $rows += "| MSVC /W4 /WX, C$std, Release build | $(if ($built) { 'built' } else { 'FAILED (see the .log file)' }) |"

    if ($built) {
        $out = ctest --test-dir $build -C Release 2>&1 | Out-String
        $line = ($out -split "`n" | Where-Object { $_ -match "tests passed|tests failed" } | Select-Object -First 1).Trim()
        $failed = ($out -split "`n" | Where-Object { $_ -match "\*\*\*Failed|Not Run" }) -join "; "
        $rows += "| MSVC, C$std, ctest (unit tests, images read back, outputs) | $line $failed |"
    }
}

Write-Output ""
Write-Output "Environment: $os, $((cmake --version | Select-Object -First 1)), vcpkg at $vcpkg"
Write-Output ""
Write-Output "| Check | Result |"
Write-Output "| --- | --- |"
$rows | ForEach-Object { Write-Output $_ }

# Raw output of the programs whose results can differ on Windows, for the article.
$bin = Join-Path $env:TEMP "libgd-msvc-c11\Release"
if (Test-Path (Join-Path $bin "pitfall_text_mode.exe")) {
    Push-Location (Join-Path $env:TEMP "libgd-msvc-c11")
    foreach ($p in "pitfall_text_mode", "pitfall_palette_limits", "test_imgtools") {
        $a = if ($p -eq "test_imgtools") { "`"$PSScriptRoot\data\downloads.txt`"" } else { "" }
        cmd /c "Release\$p.exe $a > out.txt 2> err.txt"
        Write-Output ""
        Write-Output "$p (exit $LASTEXITCODE):"
        Write-Output '```'
        Get-Content out.txt | ForEach-Object { Write-Output $_ }
        Get-Content err.txt | ForEach-Object { Write-Output "stderr: $_" }
        Write-Output '```'
    }
    Pop-Location
}
