param(
    [Parameter(Mandatory = $true)]
    [string]$ConfigPath
)

$ErrorActionPreference = 'Stop'
$config = Get-Item -LiteralPath $ConfigPath
if ($config -isnot [System.IO.FileInfo]) {
    throw "Provide a config file, such as .\rehearsal\fcfs.txt."
}

$buildDir = Join-Path $PSScriptRoot '..\build'
$exePath = Join-Path $buildDir 'csopesy.exe'
if (!(Test-Path -LiteralPath $exePath -PathType Leaf)) {
    throw "Build csopesy.exe once using the instructions in README.txt."
}
$exe = (Resolve-Path -LiteralPath $exePath).Path

$runId = [Guid]::NewGuid().ToString('N').Substring(0, 8)
$runDir = Join-Path $buildDir "rehearsal\$(Get-Date -Format 'yyyyMMdd-HHmmss')-$runId"
$run = New-Item -ItemType Directory -Path $runDir
Copy-Item -LiteralPath $config.FullName -Destination (Join-Path $run.FullName 'config.txt')

Write-Host "Using config $($config.FullName)"
Write-Host "Config and any generated report will be in $($run.FullName)"
Push-Location -LiteralPath $run.FullName
try {
    & $exe
    if ($LASTEXITCODE -ne 0) {
        throw "The emulator exited with code $LASTEXITCODE."
    }
} finally {
    Pop-Location
}
