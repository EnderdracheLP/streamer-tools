# Builds a .qmod file for loading with BMBF or QP
if ($args.Count -eq 0) {
$ModID = "streamer-tools"
$VERSION = "0.1.2"
$BSHook = "2_2_5"
$BS_Version = "1.16.4"
echo "Compiling Mod"
& $PSScriptRoot/build.ps1 --release
}

    Copy-Item "./mod_Template.json" "./mod.json" -Force
if ($args[0] -eq "--package") {
    $ModID = $env:module_id
    $BSHook = $env:bs_hook
    $BS_Version = $env:BSVersion
echo "Actions: Packaging QMod with ModID: $ModID and BS-Hook version: $BSHook"
    (Get-Content "./mod.json").replace('{VERSION_NUMBER_PLACEHOLDER}', "$env:version") | Set-Content "./mod.json"
    (Get-Content "./mod.json").replace('{BS_Hook}', "$BSHook") | Set-Content "./mod.json"
    (Get-Content "./mod.json").replace('{BS_Version}', "$BS_Version") | Set-Content "./mod.json"
    Compress-Archive -Path "./libs/arm64-v8a/lib$ModID.so", "./libs/arm64-v8a/libbeatsaber-hook_$BSHook.so", ".\mod.json", ".\cover.png" -DestinationPath "./Temp$ModID.zip" -Update
    Move-Item "./Temp$ModID.zip" "./$ModID.qmod" -Force
}
if ($? -And $args.Count -eq 0) {
echo "Packaging QMod with ModID: $ModID"
    (Get-Content "./mod.json").replace('{VERSION_NUMBER_PLACEHOLDER}', "$VERSION") | Set-Content "./mod.json"
    (Get-Content "./mod.json").replace('{BS_Hook}', "$BSHook") | Set-Content "./mod.json"
    (Get-Content "./mod.json").replace('{BS_Version}', "$BS_Version") | Set-Content "./mod.json"
    Compress-Archive -Path "./libs/arm64-v8a/lib$ModID.so", "./libs/arm64-v8a/libbeatsaber-hook_$BSHook.so", ".\mod.json", ".\cover.png" -DestinationPath "./Temp$ModID.zip" -Update
    Move-Item "./Temp$ModID.zip" "./$ModID.qmod" -Force
}
echo "Task Completed"