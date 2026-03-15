param (
    [switch]$BuildOnly,
    [switch]$SkipBuild
)

$ErrorActionPreference = "Stop"
$rootDir = (Get-Item -Path ".\").FullName
$buildDir = Join-Path $rootDir "build"
$dscExe = Join-Path $buildDir "Debug\dsc.exe" # MSVC
if (-not (Test-Path $dscExe)) {
    $dscExe = Join-Path $buildDir "dsc.exe" # MinGW/Clang
}
if (-not (Test-Path $dscExe)) {
    # Try one more just in case
    $dscExe = Join-Path $buildDir "dsc" 
}

# 1. Build the compiler
if (-not $SkipBuild) {
    Write-Host "--- Building DictatorScript Compiler ---" -ForegroundColor Cyan
    if (-not (Test-Path $buildDir)) {
        New-Item -ItemType Directory -Path $buildDir | Out-Null
    }
    
    Set-Location $buildDir
    cmake ..
    if ($LASTEXITCODE -ne 0) {
        Write-Error "CMake configuration failed."
        exit 1
    }
    
    cmake --build .
    if ($LASTEXITCODE -ne 0) {
        Write-Error "CMake build failed."
        exit 1
    }
    Set-Location $rootDir
    
    # Reload exe path in case it was just built
    $dscExe = Join-Path $buildDir "Debug\dsc.exe"
    if (-not (Test-Path $dscExe)) { $dscExe = Join-Path $buildDir "dsc.exe" }
}

if ($BuildOnly) {
    Write-Host "Build complete." -ForegroundColor Green
    exit 0
}

if (-not (Test-Path $dscExe)) {
    Write-Error "Compiler executable not found at $dscExe"
    exit 1
}

# 2. Run Tests
Write-Host "`n--- Running DictatorScript Tests ---" -ForegroundColor Cyan
$testFiles = Get-ChildItem -Path "tests" -Filter "test_*.ds"
$passed = 0
$failed = 0

foreach ($test in $testFiles) {
    Write-Host "Testing $($test.Name)..." -NoNewline
    
    # Run compiler
    $output = & $dscExe $test.FullName --emit-cpp 2>&1
    $exitCode = $LASTEXITCODE
    
    if ($exitCode -eq 0) {
        # Try to compile the generated C++ if `g++` is available
        $cppFile = $test.FullName -replace '\.ds$', '.cpp'
        $exeFile = $test.FullName -replace '\.ds$', '.exe'
        
        $compileOutput = g++ -std=c++17 $cppFile -o $exeFile 2>&1
        if ($LASTEXITCODE -eq 0) {
            Write-Host " PASSED" -ForegroundColor Green
            $passed++
            
            # Clean up generated files
            if (Test-Path $cppFile) { Remove-Item $cppFile }
            if (Test-Path $exeFile) { Remove-Item $exeFile }
        } else {
            Write-Host " FAILED (Generated C++ failed to compile)" -ForegroundColor Red
            Write-Host $compileOutput -ForegroundColor DarkGray
            $failed++
        }
    } else {
        Write-Host " FAILED (DictatorScript compilation error)" -ForegroundColor Red
        Write-Host $output -ForegroundColor DarkGray
        $failed++
    }
}

Write-Host "`n--- Test Summary ---" -ForegroundColor Cyan
Write-Host "Total: $($passed + $failed)"
Write-Host "Passed: $passed" -ForegroundColor Green
if ($failed -gt 0) {
    Write-Host "Failed: $failed" -ForegroundColor Red
    exit 1
} else {
    exit 0
}
