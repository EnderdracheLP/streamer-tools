$NDKPath = Get-Content $PSScriptRoot/ndkpath.txt
$ndkstackScript = "$NDKPath/ndk-stack"
# $SymbolsPath = Get-Content $PSScriptRoot/Symbol_SO.txt

if (-not ($PSVersionTable.PSEdition -eq "Core")) {
    $ndkstackScript += ".cmd"
}

if ($args[0] -eq "--stack") {
$timestamp = Get-Content $PSScriptRoot/log_timestamp
#& $ndkstackScript -sym ./obj/local/arm64-v8a/ -dump ./logcat.log | Tee-Object -FilePath $PSScriptRoot/log_unstripped.log
adb logcat -b crash -d -T "$timestamp" | & $ndkstackScript -sym ./obj/local/arm64-v8a/ | Tee-Object -FilePath $PSScriptRoot/log_unstripped.log
break Script
}

if ($args[0] -eq "--stack-all") {
#& $ndkstackScript -sym ./obj/local/arm64-v8a/ -dump ./logcat.log | Tee-Object -FilePath $PSScriptRoot/log_unstripped.log
adb logcat -b crash -d | & $ndkstackScript -sym ./obj/local/arm64-v8a/ | Tee-Object -FilePath $PSScriptRoot/log_unstripped.log
break Script
}

$timestamp = Get-Date -Format "MM-dd HH:mm:ss.fff"
$logger = Start-Job -Arg "$timestamp", $PSScriptRoot -scriptblock {
    param($timestamp, $Path)
    $timestamp > log_timestamp
    adb logcat -v color -T "$timestamp" main-modloader:W libmodloader:W libmain:W QuestHook[streamer-tools`|v0.1.0-InDev]:* QuestHook[streamer-tools`|v0.1.0-Dev]:* AndroidRuntime:E *:S QuestHook[UtilsLogger`|v1.3.5]:E | Tee-Object -FilePath "$Path/logcat.log"
}
[console]::TreatControlCAsInput = $true
while ($true)
{
    $buffer = Receive-Job -Job $logger
    $PermaBuffer += $buffer
    echo $buffer
    if ([console]::KeyAvailable)
    {
        $key = [system.console]::readkey($true)
        if (($key.modifiers -band [consolemodifiers]"control") -and ($key.key -eq "C"))
        {
            Add-Type -AssemblyName System.Windows.Forms
            if ([System.Windows.Forms.MessageBox]::Show("Are you sure you want to exit?", "Exit Script?", [System.Windows.Forms.MessageBoxButtons]::YesNo) -eq "Yes")
            {
                echo "Terminating logcat and getting crash dumps..."
                get-job | remove-job -Force
                adb logcat -b crash -d -T "$timestamp" | & $ndkstackScript -sym ./obj/local/arm64-v8a/ | Tee-Object -FilePath $PSScriptRoot/log_unstripped.log
                & Pause
                echo "Exiting logging.ps1"
                break
            }
        }
    }
}

