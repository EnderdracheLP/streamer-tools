$NDKPath = Get-Content $PSScriptRoot/ndkpath.txt
$PARAM = ""
for ($i = 0; $i -le $args.Length-1; $i++) {
echo "Arg $($i) is $($args[$i])"
    if ($args[$i] -eq "--actions") { $actions = $true }
    elseif ($args[$i] -eq "--debug") { $PARAM += "-DDEBUG_BUILD=1 " }
    elseif ($args[$i] -eq "--1_13_2") { $1_13_2build = $true }
    else { $PARAM += $args[$i] }
}
if ($args.Count -eq 0 -or $actions -ne $true) {
$ModID = "streamer-tools"
$VERSION = "0.1.0-InDev"
$BSHook = "2_0_3"
$codegen_ver = "0_10_2"
}


if ($actions -eq $true) {
    $ModID = $env:module_id
    $BSHook = $env:bs_hook
    $VERSION = $env:version
    $codegen_ver = $env:codegen
    if ($env:BSVersion -eq "1.13.2") { $1_13_2build = $true }
}
if ($1_13_2build -eq $true) {
echo "Making 1.13.2 Build!"
    $PARAM += " -DBS__1_13_2=1 "
    if (!$actions) {
        $BSHook = "1_1_5"
        $codegen_ver = "0_6_2"
    }
} else { $PARAM += " -DBS__1_16=1 " }
echo "Building mod with ModID: $ModID version: $VERSION, BS-Hook version: $BSHook"
Copy-Item "./Android_Template.mk" "./Android.mk" -Force
(Get-Content "./Android.mk").replace('{BS_Hook}',   "$BSHook")        | Set-Content "./Android.mk"
(Get-Content "./Android.mk").replace('{CG_VER}',    "$codegen_ver")   | Set-Content "./Android.mk"

if ($debug) {
echo "Building Debug Build!"
    $PARAM += " -DDEBUG_BUILD=1 "
}
(Get-Content "./Android.mk").replace('{DEBUG_PARAMS}',    $PARAM)   | Set-Content "./Android.mk"

$buildScript = "$NDKPath/build/ndk-build"
if (-not ($PSVersionTable.PSEdition -eq "Core")) {
    $buildScript += ".cmd"
}

& $buildScript NDK_PROJECT_PATH=$PSScriptRoot APP_BUILD_SCRIPT=$PSScriptRoot/Android.mk NDK_APPLICATION_MK=$PSScriptRoot/Application.mk -j 4 --output-sync=none -- VERSION=$VERSION

echo Done
