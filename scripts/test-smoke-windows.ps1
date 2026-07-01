$ErrorActionPreference = "Stop"

$Root = Split-Path -Parent $PSScriptRoot
Set-Location $Root

function Run-Checked {
    param(
        [Parameter(Mandatory = $true)]
        [string] $Exe,

        [Parameter(Mandatory = $true)]
        [string[]] $Args
    )

    & $Exe @Args

    if ($LASTEXITCODE -ne 0) {
        throw "Command failed: $Exe $($Args -join ' ')"
    }
}

Write-Host "Building native Release DLL..."

if (Test-Path "build-release") {
    Remove-Item -Recurse -Force "build-release"
}

Run-Checked "cmake" @(
    "-S", ".",
    "-B", "build-release",
    "-A", "x64",
    "-DRV_COPY_UNITY_PLUGIN=ON"
)

Run-Checked "cmake" @(
    "--build", "build-release",
    "--config", "Release"
)

$PluginPath = "unity\com.residual.voice\Runtime\Plugins\Windows\x86_64\residual_voice.dll"

if (!(Test-Path $PluginPath)) {
    throw "Expected Unity plugin DLL was not found: $PluginPath"
}

Write-Host "Running .NET smoke test..."

Run-Checked "dotnet" @(
    "run",
    "--project",
    "tests/ResidualVoiceSmoke"
)

Write-Host "Smoke test passed."