$timestamp = Get-Date -Format "MM-dd HH:mm:ss.fff"
adb logcat -T "$timestamp" main-modloader:W QuestHook[streamer-tools`|v0.1.0]:* AndroidRuntime:E *:S QuestHook[UtilsLogger`|v1.0.12]:E
        if ($args[0] -eq "--log") {

        }
        if ($args[0] -eq "--debug") {
            $timestamp = Get-Date -Format "MM-dd HH:mm:ss.fff"
            adb logcat -T "$timestamp" main-modloader:W QuestHook[streamer-tools`|v0.1.0]:* AndroidRuntime:E *:S QuestHook[UtilsLogger`|v1.0.12]:*
        }
echo "Exiting logging.ps1"