#include "beatsaber-hook/shared/utils/utils.h"
#include "beatsaber-hook/shared/utils/logging.hpp"
#include "beatsaber-hook/shared/utils/il2cpp-utils.hpp"
#include "beatsaber-hook/shared/utils/il2cpp-functions.hpp"
#include "beatsaber-hook/shared/utils/typedefs.h"
#include "beatsaber-hook/shared/config/config-utils.hpp"

#include "modloader/shared/modloader.hpp"

#include <string>
#include <optional>
#include "presencemanager.hpp"

static ModInfo modInfo;
static Configuration& getConfig() {
    static Configuration config(modInfo);
    return config;
}

static const Logger& getLogger() {
    static const Logger logger(modInfo);
    return logger;
}
static PresenceManager* presenceManager = nullptr;
static LevelInfo selectedLevel;

// Converts the int representing an IBeatmapDifficulty into a string
std::string difficultyToString(int difficulty)  {
    switch(difficulty)  {
        case 0:
            return "Easy";
        case 1:
            return "Normal";
        case 2:
            return "Hard";
        case 3:
            return "Expert";
        case 4:
            return "Expert+";
    }
    return "Unknown";
}

// Define the current level by finding info from the IBeatmapLevel object
MAKE_HOOK_OFFSETLESS(RefreshContent, void, Il2CppObject* self) {
    RefreshContent(self);

    Il2CppObject* level = CRASH_UNLESS(il2cpp_utils::GetFieldValue(self, "_level"));
    // Check if the level is an instance of BeatmapLevelSO
    selectedLevel.name = to_utf8(csstrtostr((Il2CppString*) CRASH_UNLESS(il2cpp_utils::GetPropertyValue(level, "songName"))));
    selectedLevel.levelAuthor = to_utf8(csstrtostr((Il2CppString*) CRASH_UNLESS(il2cpp_utils::GetPropertyValue(level, "levelAuthorName"))));
    selectedLevel.songAuthor = to_utf8(csstrtostr((Il2CppString*) CRASH_UNLESS(il2cpp_utils::GetPropertyValue(level, "songAuthorName"))));
}

int currentFrame = -1;
MAKE_HOOK_OFFSETLESS(SongStart, void, Il2CppObject* self, Il2CppString* gameMode, Il2CppObject* difficultyBeatmap, Il2CppObject* b, Il2CppObject* c, Il2CppObject* d, Il2CppObject* e, Il2CppObject* f, Il2CppString* g, bool h) {
    getLogger().info("Song Started");
    currentFrame = -1;
    int difficulty = CRASH_UNLESS(il2cpp_utils::GetPropertyValue<int>(difficultyBeatmap, "difficulty"));
    selectedLevel.selectedDifficulty = difficultyToString(difficulty);
    // Set the currently playing level to the selected one, since we are in a song
    presenceManager->statusLock.lock();
    presenceManager->playingLevel.emplace(selectedLevel);
    presenceManager->statusLock.unlock();
    SongStart(self, gameMode, difficultyBeatmap, b, c, d, e, f, g, h);
}

// Multiplayer song starting is handled differently
MAKE_HOOK_OFFSETLESS(MultiplayerSongStart, void, Il2CppObject* self, Il2CppString* gameMode, Il2CppObject* previewBeatmapLevel, int beatmapDifficulty, Il2CppObject* a, Il2CppObject* b, Il2CppObject* c, Il2CppObject* d, Il2CppObject* e, Il2CppObject* f, bool g) {
    getLogger().info("Multiplayer Song Started");
    selectedLevel.selectedDifficulty = difficultyToString(beatmapDifficulty);
    presenceManager->statusLock.lock();
    presenceManager->playingLevel.emplace(selectedLevel);
    presenceManager->statusLock.unlock();

    MultiplayerSongStart(self, gameMode, previewBeatmapLevel, beatmapDifficulty, a, b, c, d, e, f, g);
}

MAKE_HOOK_OFFSETLESS(MultiplayerJoinLobby, void, Il2CppObject* self)    {
    MultiplayerJoinLobby(self);

    getLogger().info("Joined multiplayer lobby");
    Il2CppObject* playersManager = CRASH_UNLESS(il2cpp_utils::GetFieldValue(self, "_playersManager"));
    Il2CppObject* activePlayers = CRASH_UNLESS(il2cpp_utils::GetFieldValue(playersManager, "_allActivePlayers"));
    int numActivePlayers = CRASH_UNLESS(il2cpp_utils::GetFieldValue<int>(activePlayers, "_size"));

    // Set the number of players in this lobby
    MultiplayerLobbyInfo lobbyInfo;
    lobbyInfo.numberOfPlayers = numActivePlayers;
    presenceManager->statusLock.lock();
    presenceManager->multiplayerLobby.emplace(lobbyInfo);
    presenceManager->statusLock.unlock();
}

// Reset the lobby back to null when we leave back to the menu
MAKE_HOOK_OFFSETLESS(MultiplayerLeaveLobby, void, Il2CppObject* self) {
    presenceManager->statusLock.lock();
    presenceManager->multiplayerLobby = std::nullopt;
    presenceManager->statusLock.unlock();
}

MAKE_HOOK_OFFSETLESS(SongEnd, void, Il2CppObject* self) {
    getLogger().info("Song Ended");
    presenceManager->statusLock.lock();
    presenceManager->playingLevel = std::nullopt; // Reset the currently playing song to None
    presenceManager->paused = false; // If we are pasued, unpause us, since we are returning to the menu
    presenceManager->statusLock.unlock();
    SongEnd(self);
}

MAKE_HOOK_OFFSETLESS(MultiplayerSongEnd, void, Il2CppObject* self) {
    getLogger().info("Multiplayer Song Ended");
    presenceManager->statusLock.lock();
    presenceManager->playingLevel = std::nullopt; // Reset the currently playing song to None
    presenceManager->paused = false; // If we are pasued, unpause us, since we are returning to the menu
    presenceManager->statusLock.unlock();
    SongEnd(self);
}

MAKE_HOOK_OFFSETLESS(TutorialStart, void, Il2CppObject* self)   {
    getLogger().info("Tutorial starting");
    presenceManager->statusLock.lock();
    presenceManager->playingTutorial = true;
    presenceManager->statusLock.unlock();
    TutorialStart(self);
}
MAKE_HOOK_OFFSETLESS(TutorialEnd, void, Il2CppObject* self)   {
    getLogger().info("Tutorial ending");
    presenceManager->statusLock.lock();
    presenceManager->playingTutorial = false;
    presenceManager->paused = false; // If we are pasued, unpause us, since we are returning to the menu
    presenceManager->statusLock.unlock();
    TutorialEnd(self);
}

MAKE_HOOK_OFFSETLESS(CampaignLevelStart, void, Il2CppObject* self, Il2CppObject* a, Il2CppArray* b, Il2CppObject* c, Il2CppObject* d, Il2CppObject* e, Il2CppObject* f, Il2CppString* g)   {
    getLogger().info("Campaign level starting");
    currentFrame = -1;
    presenceManager->statusLock.lock();
    presenceManager->playingCampaign = true;
    presenceManager->statusLock.unlock();
    CampaignLevelStart(self, a, b, c, d, e, f, g);
}
MAKE_HOOK_OFFSETLESS(CampaignLevelEnd, void, Il2CppObject* self)   {
    getLogger().info("Campaign level ending");
    presenceManager->statusLock.lock();
    presenceManager->playingCampaign = false;
    presenceManager->paused = false; // If we are paused, unpause us, since we are returning to the menu
    presenceManager->statusLock.unlock();
    CampaignLevelEnd(self);
}

MAKE_HOOK_OFFSETLESS(GamePause, void, Il2CppObject* self)   {
    getLogger().info("Game paused");
    presenceManager->statusLock.lock();
    presenceManager->paused = true;
    presenceManager->statusLock.unlock();
    GamePause(self);
}
MAKE_HOOK_OFFSETLESS(GameResume, void, Il2CppObject* self)   {
    getLogger().info("Game resumed");
    presenceManager->statusLock.lock();
    presenceManager->paused = false;
    presenceManager->statusLock.unlock();
    GameResume(self);
}

MAKE_HOOK_OFFSETLESS(AudioUpdate, void, Il2CppObject* self) {
    AudioUpdate(self);

    // Only update the time every 36 frames (0.5 seconds) to avoid log spam
    currentFrame++;
    if(currentFrame % 36 != 0) {return;}
    currentFrame = 0;

    float time = CRASH_UNLESS(il2cpp_utils::RunMethodUnsafe<float>(self, "get_songTime"));
    float endTime = CRASH_UNLESS(il2cpp_utils::RunMethodUnsafe<float>(self, "get_songEndTime"));

    presenceManager->statusLock.lock();
    presenceManager->timeLeft = (int) (endTime - time);
    presenceManager->statusLock.unlock();
}

void saveDefaultConfig()  {
    getLogger().info("Creating config file . . .");
    ConfigDocument& config = getConfig().config;
    auto& alloc = config.GetAllocator();
    // If the config has already been created, don't overwrite it
    if(config.HasMember("multiplayerLevelPresence")) {
        getLogger().info("Config file already exists");
        return;
    }
    config.RemoveAllMembers();
    config.SetObject();
    // Create the sections of the config file for each type of presence
    rapidjson::Value levelPresence(rapidjson::kObjectType);
    levelPresence.AddMember("details", "Playing {mapName} ({mapDifficulty})", alloc);
    levelPresence.AddMember("state",  "By: {mapAuthor} {paused?}", alloc);
    config.AddMember("standardLevelPresence", levelPresence, alloc);

    rapidjson::Value multiLevelPresence(rapidjson::kObjectType);
    multiLevelPresence.AddMember("details", "Playing multiplayer with {numPlayers} others.", alloc);
    multiLevelPresence.AddMember("state",  "{mapName} - {mapDifficulty} {paused?}", alloc);
    config.AddMember("multiplayerLevelPresence", multiLevelPresence, alloc);

    rapidjson::Value missionPresence(rapidjson::kObjectType);
    missionPresence.AddMember("details", "Playing Campaign", alloc);
    missionPresence.AddMember("state",  "{paused?}", alloc);
    config.AddMember("missionLevelPresence", missionPresence, alloc);

    rapidjson::Value tutorialPresence(rapidjson::kObjectType);
    tutorialPresence.AddMember("details", "Playing Tutorial", alloc);
    tutorialPresence.AddMember("state",  "{paused?}", alloc);
    config.AddMember("tutorialPresence", tutorialPresence, alloc);

    rapidjson::Value multiLobbyPresence(rapidjson::kObjectType);
    multiLobbyPresence.AddMember("details", "Multiplayer - In Lobby", alloc);
    multiLobbyPresence.AddMember("state",  "with {numPlayers} others", alloc);
    config.AddMember("multiplayerLobbyPresence", multiLobbyPresence, alloc);

    rapidjson::Value menuPresence(rapidjson::kObjectType);
    menuPresence.AddMember("details", "In Menu", alloc);
    menuPresence.AddMember("state",  "", alloc);
    config.AddMember("menuPresence", menuPresence, alloc);

    getConfig().Write();
    getLogger().info("Config file created");
}

extern "C" void setup(ModInfo& info) {
    info.id = "discord-presence";
    info.version = "0.1.3";
    modInfo = info;
    getLogger().info("Modloader name: %s", Modloader::getInfo().name.c_str());
    getConfig().Load();
    saveDefaultConfig(); // Create the default config file

    getLogger().info("Completed setup!");
}

extern "C" void load() {
    getLogger().debug("Installing hooks...");
    il2cpp_functions::Init();

    // Install our function hooks
    INSTALL_HOOK_OFFSETLESS(RefreshContent, il2cpp_utils::FindMethodUnsafe("", "StandardLevelDetailView", "RefreshContent", 0));
    INSTALL_HOOK_OFFSETLESS(SongStart, il2cpp_utils::FindMethodUnsafe("", "StandardLevelScenesTransitionSetupDataSO", "Init", 9));
    INSTALL_HOOK_OFFSETLESS(SongEnd, il2cpp_utils::FindMethodUnsafe("", "StandardLevelGameplayManager", "OnDestroy", 0));
    INSTALL_HOOK_OFFSETLESS(CampaignLevelStart, il2cpp_utils::FindMethodUnsafe("", "MissionLevelScenesTransitionSetupDataSO", "Init", 7));
    INSTALL_HOOK_OFFSETLESS(CampaignLevelEnd, il2cpp_utils::FindMethodUnsafe("", "MissionLevelGameplayManager", "OnDestroy", 0));
    INSTALL_HOOK_OFFSETLESS(TutorialStart, il2cpp_utils::FindMethodUnsafe("", "TutorialSongController", "Awake", 0));
    INSTALL_HOOK_OFFSETLESS(TutorialEnd, il2cpp_utils::FindMethodUnsafe("", "TutorialSongController", "OnDestroy", 0));
    INSTALL_HOOK_OFFSETLESS(GamePause, il2cpp_utils::FindMethodUnsafe("", "PauseController", "Pause", 0));
    INSTALL_HOOK_OFFSETLESS(GameResume, il2cpp_utils::FindMethodUnsafe("", "PauseController", "HandlePauseMenuManagerDidPressContinueButton", 0));
    INSTALL_HOOK_OFFSETLESS(AudioUpdate, il2cpp_utils::FindMethodUnsafe("", "AudioTimeSyncController", "Update", 0));
    INSTALL_HOOK_OFFSETLESS(MultiplayerSongStart, il2cpp_utils::FindMethodUnsafe("", "MultiplayerLevelScenesTransitionSetupDataSO", "Init", 10));
    INSTALL_HOOK_OFFSETLESS(MultiplayerJoinLobby, il2cpp_utils::FindMethodUnsafe("", "MultiplayerController", "Start", 0));
    INSTALL_HOOK_OFFSETLESS(MultiplayerSongEnd, il2cpp_utils::FindMethodUnsafe("", "MultiplayerLocalActivePlayerGameplayManager", "OnDisable", 0));
    INSTALL_HOOK_OFFSETLESS(MultiplayerLeaveLobby, il2cpp_utils::FindMethodUnsafe("", "MultiplayerController", "OnDestroy", 0));

    getLogger().debug("Installed all hooks!");
    presenceManager = new PresenceManager(getLogger(), getConfig().config);
}