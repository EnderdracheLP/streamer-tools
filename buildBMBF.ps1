# Builds a .zip file for loading with BMBF
$NDKPath = Get-Content $PSScriptRoot/ndkpath.txt

$buildScript = "$NDKPath/build/ndk-build"
if (-not ($PSVersionTable.PSEdition -eq "Core")) {
    $buildScript += ".cmd"
}

& $buildScript NDK_PROJECT_PATH=$PSScriptRoot APP_BUILD_SCRIPT=$PSScriptRoot/Android.mk NDK_APPLICATION_MK=$PSScriptRoot/Application.mk
Compress-Archive -Path "./libs/arm64-v8a/libstreamer-tools.so", "cover.png", "./bmbfmod.json" -DestinationPath "./streamer-tools_v0.1.0.zip" -Update

# Builds a .zip file for loading with BMBF
if ($args.Count -eq 0) {
$ModID = "streamer-tools"
$VERSION = "0.1.0"
$BSHook = "1_1_5"
$BS_Version = "1.13.2"
echo "Compiling Mod"
& $PSScriptRoot/build.ps1
}

    Copy-Item "./bmbfmod_Template.json" "./bmbfmod.json" -Force
if ($args[0] -eq "--package") {
    $ModID = $env:module_id
    $BSHook = $env:bs_hook
    $BS_Version = $env:BSVersion
echo "Actions: Packaging QMod with ModID: $ModID and BS-Hook version: $BSHook"
    (Get-Content "./bmbfmod.json").replace('{VERSION_NUMBER_PLACEHOLDER}', "$env:version") | Set-Content "./bmbfmod.json"
    (Get-Content "./bmbfmod.json").replace('{BS_Hook}', "$BSHook") | Set-Content "./bmbfmod.json"
    (Get-Content "./bmbfmod.json").replace('{BS_Version}', "$BS_Version") | Set-Content "./bmbfmod.json"
    Compress-Archive -Path "./libs/arm64-v8a/lib$ModID.so", "./libs/arm64-v8a/libbeatsaber-hook_$BSHook.so", ".\bmbfmod.json", ".\cover.png", "./libs/arm64-v8a/libsocket_lib.so" -DestinationPath "./$ModID.zip" -Update
}
if ($? -And $args.Count -eq 0) {
echo "Packaging QMod with ModID: $ModID"
    (Get-Content "./bmbfmod.json").replace('{VERSION_NUMBER_PLACEHOLDER}', "$VERSION") | Set-Content "./bmbfmod.json"
    (Get-Content "./bmbfmod.json").replace('{BS_Hook}', "$BSHook") | Set-Content "./bmbfmod.json"
    (Get-Content "./bmbfmod.json").replace('{BS_Version}', "$BS_Version") | Set-Content "./bmbfmod.json"
    Compress-Archive -Path "./libs/arm64-v8a/lib$ModID.so", "./libs/arm64-v8a/libbeatsaber-hook_$BSHook.so", ".\bmbfmod.json", ".\cover.png", "./libs/arm64-v8a/libsocket_lib.so" -DestinationPath "./$ModID.zip" -Update
}
echo "Task Completed"