# run_all.ps1 - the Windows checks behind the article's "What I Verified" table.
#   1. Builds with MSVC (x64, C11 and C17, /W4 /WX) and runs CTest.
#   2. Builds with AddressSanitizer (/fsanitize=address) and checks that it
#      stops the off-by-one and read-after-free programs.
#   3. Applies deliberate mistakes that a test must catch.
# Run from this directory in a Developer PowerShell:  .\run_all.ps1
Set-Location $PSScriptRoot
$results = @()
function Row($name, $text) { $script:results += ("{0,-58} {1}" -f $name, $text) }

function Check($src, $dir, $extra, $log) {
    if (Test-Path $dir) { Remove-Item -Recurse -Force $dir }
    cmake -S $src -B $dir -A x64 @extra *> "$log-build.log"
    if ($LASTEXITCODE -ne 0) { return "configure-failed" }
    cmake --build $dir --config Release *>> "$log-build.log"
    if ($LASTEXITCODE -ne 0) { return "build-failed" }
    Remove-Item "$log-build.log"
    ctest --test-dir $dir -C Release --output-on-failure *> "$log-ctest.log"
    if ($LASTEXITCODE -ne 0) { return "tests-failed" }
    Remove-Item "$log-ctest.log"
    return "passed"
}

$cl = Get-Command cl.exe -ErrorAction SilentlyContinue
$results += "Environment: $([System.Environment]::OSVersion.VersionString)"
if ($cl) { $results += "cl.exe $($cl.FileVersionInfo.FileVersion)" } else { $results += "cl.exe not on PATH: run this in a Developer PowerShell" }
foreach ($std in 11, 17) {
    $r = Check "." "build-c$std" @("-DCMAKE_C_STANDARD=$std") "msvc-c$std"
    Row "MSVC C$std x64 build and CTest (/W4 /WX)" $(if ($r -eq "passed") { "PASS" } else { $r })
}
if (Test-Path "build-c11\Release\test_defensive.exe") {
    $results += (& "build-c11\Release\test_defensive.exe" data $env:TEMP | Select-Object -Last 1)
}

# AddressSanitizer. ASan programs need the x64 ASan runtime DLL on PATH. A
# Developer PowerShell starts with the x86 tools on PATH, so add the x64 folder.
$savedPath = $env:PATH
if ($cl) {
    $bin = Split-Path (Split-Path (Split-Path $cl.Source))   # ...\bin
    $dll = Get-ChildItem -Path (Join-Path $bin "Hostx64\x64"), (Join-Path $bin "Hostx86\x64") -Filter "clang_rt.asan_dynamic-x86_64.dll" -ErrorAction SilentlyContinue | Select-Object -First 1
    if ($dll) { $env:PATH = "$($dll.DirectoryName);$env:PATH" } else { Row "ASan runtime DLL" "not found under $bin" }
}
$r = Check "." "build-asan" @("-DCMAKE_C_FLAGS=/fsanitize=address /Zi") "msvc-asan"
Row "MSVC AddressSanitizer build and CTest" $(if ($r -eq "passed") { "PASS" } else { $r })
foreach ($p in @(@("pitfall_off_by_one", "heap-buffer-overflow"), @("pitfall_use_after_free", "heap-use-after-free"))) {
    $exe = "build-asan\Release\$($p[0]).exe"
    if (-not (Test-Path $exe)) { continue }
    $out = (& cmd /c "`"$exe`" 2>&1") -join "`n"
    $found = [regex]::Match($out, "AddressSanitizer: [a-z-]+").Value
    Row "ASan: $($p[0])" $(if ($found -match $p[1]) { $found } elseif ($found) { "unexpected: $found" } else { "NOT DETECTED" })
}
$env:PATH = $savedPath

# Deliberate mistakes. A mutant only counts if the unchanged code passed.
$mutants = @(
    @("if (*end != '\0') {", "if (0) {", "text after a number accepted"),
    @("if (c != '\n' && c != EOF) {", "if (1) {", "a line that fills the buffer called too long"),
    @("if (b != 0 && a > SIZE_MAX / b) {", "if (0) {", "no overflow check before multiplying"),
    @("if (status != DP_OK && created) {", "if (status != DP_OK) {", "an existing file deleted after a failed copy")
)
if (($results -match "^MSVC C") -notmatch "PASS$") { Row "Mutants" "not run: the unchanged code fails its tests"; $mutants = @() }
$n = 0
foreach ($m in $mutants) {
    $n++
    $copy = Join-Path $env:TEMP "defensive-mutant-$n"
    if (Test-Path $copy) { Remove-Item -Recurse -Force $copy }
    New-Item -ItemType Directory $copy | Out-Null
    Copy-Item -Recurse CMakeLists.txt, src, tests, data, pitfalls, compile_fail $copy
    $file = Join-Path $copy "src\defensive.c"
    $text = [System.IO.File]::ReadAllText($file)
    if (-not $text.Contains($m[0])) { Row "Mutant ${n}: $($m[2])" "NOT APPLIED"; continue }
    [System.IO.File]::WriteAllText($file, $text.Replace($m[0], $m[1]))
    $r = Check $copy (Join-Path $copy "build") @() "msvc-mutant-$n"
    $verdict = switch ($r) { "tests-failed" { "CAUGHT" } "passed" { "MISSED" } default { "BUILD FAILED" } }
    Remove-Item -ErrorAction SilentlyContinue "msvc-mutant-$n-ctest.log"
    Row "Mutant ${n}: $($m[2])" $verdict
    Remove-Item -Recurse -Force $copy
}

$results | Tee-Object -FilePath results-windows.md
