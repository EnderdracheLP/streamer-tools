# Builds a .zip file for loading with BMBF
$NDKPath = Get-Content $PSScriptRoot/ndkpath.txt

$buildScript = "$NDKPath/build/ndk-build"
if (-not ($PSVersionTable.PSEdition -eq "Core")) {
    $buildScript += ".cmd"
}

& $buildScript NDK_PROJECT_PATH=$PSScriptRoot APP_BUILD_SCRIPT=$PSScriptRoot/Android.mk NDK_APPLICATION_MK=$PSScriptRoot/Application.mk
Compress-Archive -Path "./libs/arm64-v8a/libdiscord-presence.so", "./libs/arm64-v8a/libbeatsaber-hook_1_0_9.so", "./libs/arm64-v8a/libcodegen_0_5_3.so", "cover.png", "./bmbfmod.json", "module.json" -DestinationPath "./discord-presence_v0.2.1.zip" -Update
