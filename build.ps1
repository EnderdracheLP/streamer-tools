$NDKPath = Get-Content $PSScriptRoot/ndkpath.txt
if ($args.Count -eq 0 -or $args[0] -eq "--debug") {
$ModID = "streamer-tools"
$VERSION = "0.1.0-InDev"
$BSHook = "1_3_5"
$codegen_ver = "0_9_0"
}

for ($i = 0; $args.Length > $i; $i++) {
    if ($args[$i] -eq "--actions") { $actions = true }
    elseif ($args[$i] -eq "--debug") { $PARAM += "-DDEBUG_BUILD=1" }
    else { $PARAM += $args[$i] }

}

if ($actions) {
    $ModID = $env:module_id
    $BSHook = $env:bs_hook
    $VERSION = $env:version
    $codegen_ver = $env:codegen
}
echo "Building mod with ModID: $ModID version: $VERSION, BS-Hook version: $BSHook"
# Copy-Item "./Android_Template.mk" "./Android.mk" -Force
(Get-Content "./Android.mk").replace('{BS_Hook}',   "$BSHook")        | Set-Content "./Android.mk"
(Get-Content "./Android.mk").replace('{CG_VER}',    "$codegen_ver")   | Set-Content "./Android.mk"

if ($debug) {
    $PARAM = "-DDEBUG_BUILD=1"
} else {
    $PARAM = ""
}
# (Get-Content "./Android.mk").replace('{DEBUG_PARAMS}',    $PARAM)   | Set-Content "./Android.mk"

$buildScript = "$NDKPath/build/ndk-build"
if (-not ($PSVersionTable.PSEdition -eq "Core")) {
    $buildScript += ".cmd"
}

& $buildScript -j4 NDK_PROJECT_PATH=$PSScriptRoot APP_BUILD_SCRIPT=$PSScriptRoot/Android.mk NDK_APPLICATION_MK=$PSScriptRoot/Application.mk VERSION=$VERSION $PARAM

echo Done
