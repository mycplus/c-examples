# Build and test with MSVC. Run:  .\run_all.ps1
# Results go to the screen; redirect them to results-windows.md if you want a file to send back.
$ErrorActionPreference = 'Continue'
$failed = 0
foreach ($std in 11,17) {
    $dir = "build-c$std"
    if (Test-Path $dir) { Remove-Item -Recurse -Force $dir }
    cmake -S . -B $dir "-DCMAKE_C_STANDARD=$std" *> "msvc-configure-c$std.log"
    if ($LASTEXITCODE -ne 0) { Write-Output "c$std : configure FAILED (see msvc-configure-c$std.log)"; $failed++; continue }
    cmake --build $dir --config Release *> "msvc-build-c$std.log"
    if ($LASTEXITCODE -ne 0) { Write-Output "c$std : build FAILED (see msvc-build-c$std.log)"; $failed++; continue }
    ctest --test-dir $dir -C Release --output-on-failure *> "msvc-test-c$std.log"
    if ($LASTEXITCODE -ne 0) { Write-Output "c$std : tests FAILED (see msvc-test-c$std.log)"; $failed++ }
    else { Write-Output "c$std : MSVC build and tests passed" }
}
exit $failed
