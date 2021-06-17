$timestamp = Get-Date -Format "MM-dd HH:mm:ss.fff"
adb logcat -T "$timestamp" main-modloader:W libmodloader:W libmain:W QuestHook[streamer-tools`|v0.1.0-InDev]:* QuestHook[streamer-tools`|v0.1.0-Dev]:* QuestHook[SongLoader`|v0.3.0]:* AndroidRuntime:E *:S QuestHook[UtilsLogger`|v1.3.5]:E | Tee-Object -FilePath $PSScriptRoot/logcat.log
echo "Exiting logging.ps1"