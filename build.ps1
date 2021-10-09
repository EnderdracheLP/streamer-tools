$NDKPath = Get-Content $PSScriptRoot/ndkpath.txt
$PARAM = ""
for ($i = 0; $i -le $args.Length-1; $i++) {
echo "Arg $($i) is $($args[$i])"
    if ($args[$i] -eq "--actions") { $actions = $true }
    elseif ($args[$i] -eq "--debug") { $PARAM += "-DDEBUG_BUILD=1 " }
    elseif ($args[$i] -eq "--1_16_4") { $1_16_4build = $true }
    elseif ($args[$i] -eq "--1_16_2") { $1_16_2build = $true }
    elseif ($args[$i] -eq "--1_13_2") { $1_13_2build = $true }
    elseif ($args[$i] -eq "--release") { $release = $true }
    else { $PARAM += $args[$i] }
}
if ($args.Count -eq 0 -or $actions -ne $true) {
$ModID = "streamer-tools"
$VERSION = "0.2.1"
if ($release -ne $true) {
    $VERSION += "-InDev"
}

    if ($1_16_2build -eq $true) {
    echo "1.16.2 Build!"
        $BSHook = "2_2_2"
        $codegen_ver = "0_12_3"
        $BS_VERSION_PATCH = 2
    }
    elseif ($1_16_4build -eq $true) {
    echo "1.16.4 Build!"
    $BSHook = "2_2_5"
    $codegen_ver = "0_12_5"
    $BS_VERSION_PATCH = 4
    }
    else {
    echo "1.17.1 Build!"
    $BSHook = "2_3_0"
    $BS_VERSION_PATCH = 0
    }
}

if ($actions -eq $true) {
    $ModID = $env:module_id
    $BSHook = $env:bs_hook
    $VERSION = $env:version
    $codegen_ver = $env:codegen
    if ($env:BSVersion -eq "1.17.1") {$BS_VERSION_PATCH = 1}
    elseif ($env:BSVersion -eq "1.16.4") {$BS_VERSION_PATCH = 4}
    elseif ($env:BSVersion -eq "1.16.2") {$BS_VERSION_PATCH = 2}
    elseif ($env:BSVersion -eq "1.13.2") { $1_13_2build = $true }
}
if ($1_13_2build -eq $true) {
echo "Making 1.13.2 Build!"
    $PARAM += " -DBS__1_13_2=1 "
    if (!$actions) {
        $BSHook = "1_0_12"
        $codegen_ver = "0_6_2"
    }
} elseif ($1_16_4build -eq $true -or $1_16_2build -eq $true) { $PARAM += " -DBS__1_16=$BS_VERSION_PATCH " }
else { $PARAM += " -DBS__1_17=$BS_VERSION_PATCH " }
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

& $buildScript NDK_PROJECT_PATH=$PSScriptRoot APP_BUILD_SCRIPT=$PSScriptRoot/Android.mk NDK_APPLICATION_MK=$PSScriptRoot/Application.mk -j 6 --output-sync=none -- VERSION=$VERSION

echo Done
