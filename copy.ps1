if ($args[0] -eq "--debug" -or $args[1] -eq "--debug") {
& $PSScriptRoot/build.ps1 --debug
} else {
& $PSScriptRoot/build.ps1
}
if ($?) {
    adb push libs/arm64-v8a/libstreamer-tools.so /sdcard/Android/data/com.beatgames.beatsaber/files/mods/libstreamer-tools.so
    if ($?) {
        adb shell am force-stop com.beatgames.beatsaber
        adb shell am start com.beatgames.beatsaber/com.unity3d.player.UnityPlayerActivity
        if ($args[0] -eq "--log" -or $args[1] -eq "--log") {
            $timestamp = Get-Date -Format "MM-dd HH:mm:ss.fff"
            Start-Process PowerShell -ArgumentList "./logging.ps1 --file"
        }
        if ($args[0] -eq "--debug-log" -or $args[1] -eq "--debug-log") {
            $timestamp = Get-Date -Format "MM-dd HH:mm:ss.fff"
            adb logcat -T "$timestamp" main-modloader:W QuestHook[streamer-tools`|v0.1.0]:* AndroidRuntime:E *:S QuestHook[UtilsLogger`|v1.2.4]:*
        }
    }
}
echo "Exiting Copy.ps1"