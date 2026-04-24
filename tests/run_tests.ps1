
param (
    [switch]$BuildOnly,
    [switch]$SkipBuild,
    [switch]$Verbose,
    [string]$TestFilter = "test_*.ds",
    [switch]$KeepArtifacts
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

# ── Colour helpers ────────────────────────────────────────────────────────────
function Write-Header  ([string]$msg) { Write-Host "`n$("─" * 50)" -ForegroundColor DarkGray
                                        Write-Host "  $msg" -ForegroundColor Cyan
                                        Write-Host "$("─" * 50)" -ForegroundColor DarkGray }
function Write-Pass    ([string]$msg) { Write-Host " ✓  $msg" -ForegroundColor Green  }
function Write-Fail    ([string]$msg) { Write-Host " ✗  $msg" -ForegroundColor Red    }
function Write-Info    ([string]$msg) { Write-Host "    $msg" -ForegroundColor DarkGray }
function Write-Warn    ([string]$msg) { Write-Host "  ! $msg" -ForegroundColor Yellow  }

# ── Path resolution ───────────────────────────────────────────────────────────
$rootDir  = (Get-Item -Path ".\").FullName
$buildDir = Join-Path $rootDir "build"
$testsDir = Join-Path $rootDir "tests"

function Resolve-CompilerExe {
    $candidates = @(
        (Join-Path $buildDir "Debug\dsc.exe"),   # MSVC Debug
        (Join-Path $buildDir "Release\dsc.exe"), # MSVC Release
        (Join-Path $buildDir "dsc.exe"),          # MinGW / Clang on Windows
        (Join-Path $buildDir "dsc")              # Linux / macOS
    )
    foreach ($c in $candidates) {
        if (Test-Path $c) { return $c }
    }
    return $null
}

# ── 1. Build ──────────────────────────────────────────────────────────────────
if (-not $SkipBuild) {
    Write-Header "Building DictatorScript Compiler"

    if (-not (Test-Path $buildDir)) {
        New-Item -ItemType Directory -Path $buildDir | Out-Null
    }

    Push-Location $buildDir
    try {
        Write-Info "Running cmake configuration..."
        cmake .. | ForEach-Object { if ($Verbose) { Write-Info $_ } }
        if ($LASTEXITCODE -ne 0) { throw "CMake configuration failed (exit $LASTEXITCODE)." }

        Write-Info "Building..."
        cmake --build . | ForEach-Object { if ($Verbose) { Write-Info $_ } }
        if ($LASTEXITCODE -ne 0) { throw "CMake build failed (exit $LASTEXITCODE)." }
    }
    finally {
        Pop-Location
    }

    Write-Pass "Build succeeded."
}

if ($BuildOnly) {
    Write-Host "`nBuild-only mode — exiting." -ForegroundColor Green
    exit 0
}

# ── 2. Locate compiler ────────────────────────────────────────────────────────
$dscExe = Resolve-CompilerExe
if (-not $dscExe) {
    Write-Error "Compiler executable not found under '$buildDir'. Run without -SkipBuild or check your build output."
    exit 1
}
Write-Info "Using compiler: $dscExe"

# ── 3. Discover test files ────────────────────────────────────────────────────
if (-not (Test-Path $testsDir)) {
    Write-Error "Tests directory not found: $testsDir"
    exit 1
}

$testFiles = @(Get-ChildItem -Path $testsDir -Filter $TestFilter -File)
if ($testFiles.Count -eq 0) {
    Write-Warn "No test files matched filter '$TestFilter' in '$testsDir'."
    exit 0
}

# ── 4. Detect g++ ─────────────────────────────────────────────────────────────
$gppAvailable = $null -ne (Get-Command "g++" -ErrorAction SilentlyContinue)
if (-not $gppAvailable) {
    Write-Warn "g++ not found on PATH — generated C++ will not be compiled."
}

# ── 5. Run tests ──────────────────────────────────────────────────────────────
Write-Header "Running DictatorScript Tests  ($($testFiles.Count) found)"

$results   = [System.Collections.Generic.List[PSCustomObject]]::new()
$stopwatch = [System.Diagnostics.Stopwatch]::StartNew()

foreach ($test in $testFiles) {
    $label     = $test.Name
    $cppFile   = [System.IO.Path]::ChangeExtension($test.FullName, ".cpp")
    $exeFile   = [System.IO.Path]::ChangeExtension($test.FullName, ".exe")
    $sw        = [System.Diagnostics.Stopwatch]::StartNew()

    Write-Host "  ► $label" -NoNewline -ForegroundColor White

    # Stage A — transpile .ds → .cpp
    $dscOutput = & $dscExe $test.FullName --emit-cpp 2>&1
    $dscExit   = $LASTEXITCODE
    $sw.Stop()

    if ($dscExit -ne 0) {
        Write-Fail "  ← DSC error  ($($sw.ElapsedMilliseconds) ms)"
        if ($Verbose -or $true) {   # always show DSC errors
            $dscOutput | ForEach-Object { Write-Info "    $_" }
        }
        $results.Add([PSCustomObject]@{ Name=$label; Status="FAIL_DSC";    Ms=$sw.ElapsedMilliseconds })
        continue
    }

    if (-not $gppAvailable) {
        Write-Host " ✓ (transpiled)" -ForegroundColor Yellow
        $results.Add([PSCustomObject]@{ Name=$label; Status="TRANSPILED"; Ms=$sw.ElapsedMilliseconds })
        continue
    }

    # Stage B — compile generated .cpp
    $gppOutput = g++ -std=c++17 $cppFile -o $exeFile 2>&1
    if ($LASTEXITCODE -ne 0) {
        Write-Fail "  ← g++ error   ($($sw.ElapsedMilliseconds) ms)"
        if ($Verbose) { $gppOutput | ForEach-Object { Write-Info "    $_" } }
        $results.Add([PSCustomObject]@{ Name=$label; Status="FAIL_GPP";   Ms=$sw.ElapsedMilliseconds })
    }
    else {
        Write-Pass "  ($($sw.ElapsedMilliseconds) ms)"
        $results.Add([PSCustomObject]@{ Name=$label; Status="PASS";        Ms=$sw.ElapsedMilliseconds })
    }

    # Cleanup
    if (-not $KeepArtifacts) {
        @($cppFile, $exeFile) | Where-Object { Test-Path $_ } | Remove-Item -Force
    }
}

$stopwatch.Stop()

# ── 6. Summary ────────────────────────────────────────────────────────────────
Write-Header "Test Summary"

$passed      = @($results | Where-Object { $_.Status -eq "PASS"        }).Count
$transpiled  = @($results | Where-Object { $_.Status -eq "TRANSPILED"  }).Count
$failDsc     = @($results | Where-Object { $_.Status -eq "FAIL_DSC"    }).Count
$failGpp     = @($results | Where-Object { $_.Status -eq "FAIL_GPP"    }).Count
$total       = $results.Count
$failures    = $failDsc + $failGpp

$colW = 28
Write-Host ("  {0,-$colW} {1,6}" -f "Total tests",          $total)           -ForegroundColor White
Write-Host ("  {0,-$colW} {1,6}" -f "Passed",               $passed)          -ForegroundColor Green
if ($transpiled -gt 0) {
Write-Host ("  {0,-$colW} {1,6}" -f "Transpiled (no g++)", $transpiled)       -ForegroundColor Yellow }
if ($failDsc   -gt 0) {
Write-Host ("  {0,-$colW} {1,6}" -f "Failed (DSC stage)",  $failDsc)          -ForegroundColor Red    }
if ($failGpp   -gt 0) {
Write-Host ("  {0,-$colW} {1,6}" -f "Failed (g++ stage)",  $failGpp)          -ForegroundColor Red    }
Write-Host ("  {0,-$colW} {1,6}" -f "Total time (ms)",     $stopwatch.ElapsedMilliseconds) -ForegroundColor DarkGray

if ($failures -gt 0) {
    Write-Host "`n  Failed tests:" -ForegroundColor Red
    $results | Where-Object { $_.Status -like "FAIL*" } |
        ForEach-Object { Write-Host "    • $($_.Name)  [$($_.Status)]" -ForegroundColor DarkRed }
    Write-Host ""
    exit 1
}

Write-Host "`n  All tests passed.`n" -ForegroundColor Green
exit 0
