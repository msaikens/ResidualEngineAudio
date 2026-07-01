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
$SmokeProject = "tests\ResidualVoiceSmoke\ResidualVoiceSmoke.csproj"
$SmokeOutputDir = "tests\ResidualVoiceSmoke\bin\Debug\net8.0"
$SmokeDllPath = Join-Path $SmokeOutputDir "residual_voice.dll"

if (!(Test-Path $PluginPath)) {
    throw "Expected Unity plugin DLL was not found: $PluginPath"
}

Write-Host "Validating Unity package layout..."

$RequiredPackageFiles = @(
    "unity\com.residual.voice\package.json",
    "unity\com.residual.voice\Runtime\ResidualVoice.asmdef",
    "unity\com.residual.voice\Runtime\Scripts\ResidualVoiceNative.cs",
    "unity\com.residual.voice\Runtime\Scripts\ResidualVoiceConfig.cs",
    "unity\com.residual.voice\Runtime\Scripts\ResidualVoiceClient.cs",
    "unity\com.residual.voice\Runtime\Scripts\IResidualVoiceTransport.cs",
    "unity\com.residual.voice\Runtime\Scripts\ResidualVoiceLoopbackTransport.cs",
    "unity\com.residual.voice\Runtime\Scripts\ResidualVoiceTransportBinding.cs",
    "unity\com.residual.voice\Runtime\Scripts\ResidualVoicePcmUtility.cs"
)

foreach ($File in $RequiredPackageFiles) {
    if (!(Test-Path $File)) {
        throw "Required Unity package file is missing: $File"
    }
}

Write-Host "Building .NET smoke test..."

Run-Checked "dotnet" @(
    "build",
    $SmokeProject
)

if (!(Test-Path $SmokeOutputDir)) {
    throw "Expected smoke test output directory was not found: $SmokeOutputDir"
}

Write-Host "Copying native DLL beside smoke test executable..."

Copy-Item $PluginPath $SmokeDllPath -Force

if (!(Test-Path $SmokeDllPath)) {
    throw "Native DLL was not copied to smoke test output: $SmokeDllPath"
}

Write-Host "Running .NET smoke test..."

Run-Checked "dotnet" @(
    "run",
    "--no-build",
    "--project",
    $SmokeProject
)

Write-Host "Smoke test passed."