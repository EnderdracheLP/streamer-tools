#define NO_CODEGEN_USE

#include "beatsaber-hook/shared/utils/typedefs.h"
#include "beatsaber-hook/shared/utils/il2cpp-functions.hpp"
#include "beatsaber-hook/shared/utils/utils.h"
#include "beatsaber-hook/shared/utils/logging.hpp"
#include "beatsaber-hook/shared/utils/il2cpp-utils.hpp"
#include "beatsaber-hook/shared/utils/typedefs.h"
#include "beatsaber-hook/shared/config/config-utils.hpp"

#include "UnityEngine/Resources.hpp"

#include "System/Action_1.hpp"
#include "System/Action_2.hpp"
#include "System/Action_3.hpp"

#include "GlobalNamespace/IConnectedPlayer.hpp"
#include "GlobalNamespace/MultiplayerPlayersManager.hpp"
#include "GlobalNamespace/MultiplayerSessionManager.hpp"
#include "GlobalNamespace/GameServerLobbyFlowCoordinator.hpp"
#include "GlobalNamespace/PracticeSettings.hpp"
#include "GlobalNamespace/ScoreController.hpp"  // Added for getting Score Information.
using namespace GlobalNamespace;

#include "modloader/shared/modloader.hpp"

#include <string>
#include <optional>
#include "presencemanager.hpp"

static ModInfo modInfo;
static Configuration& getConfig() {
    static Configuration config(modInfo);
    return config;
}

static Logger& getLogger() {
    static Logger* logger = new Logger(modInfo);
    return *logger;
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
    selectedLevel.nameSub = to_utf8(csstrtostr((Il2CppString*)CRASH_UNLESS(il2cpp_utils::GetPropertyValue(level, "songSubName"))));
    selectedLevel.levelAuthor = to_utf8(csstrtostr((Il2CppString*) CRASH_UNLESS(il2cpp_utils::GetPropertyValue(level, "levelAuthorName"))));
    selectedLevel.songAuthor = to_utf8(csstrtostr((Il2CppString*) CRASH_UNLESS(il2cpp_utils::GetPropertyValue(level, "songAuthorName"))));
    selectedLevel.id = to_utf8(csstrtostr((Il2CppString*)CRASH_UNLESS(il2cpp_utils::GetPropertyValue(level, "levelID"))));
}

int currentFrame = -1;
MAKE_HOOK_OFFSETLESS(SongStart, void, Il2CppObject* self, Il2CppString* gameMode, Il2CppObject* difficultyBeatmap, Il2CppObject* b, Il2CppObject* c, Il2CppObject* d, Il2CppObject* e, PracticeSettings* practiceSettings, Il2CppString* g, bool h) {
    getLogger().info("Song Started");
    currentFrame = -1;
    int difficulty = CRASH_UNLESS(il2cpp_utils::GetPropertyValue<int>(difficultyBeatmap, "difficulty"));
    selectedLevel.selectedDifficulty = difficultyToString(difficulty);

    // Set the currently playing level to the selected one, since we are in a song
    presenceManager->statusLock.lock();
    presenceManager->playingLevel.emplace(selectedLevel);
    presenceManager->isPractice = practiceSettings; // If practice settings isn't null, then we're in practice mode
    presenceManager->statusLock.unlock();

    if(presenceManager->isPractice) {
        getLogger().info("Practice mode is enabled!");
    }
    SongStart(self, gameMode, difficultyBeatmap, b, c, d, e, practiceSettings, g, h);
}
// TODO: Add getting scores
/* // Code for getting score and other stuff, (Placeholder, needs correct implementation.)
MAKE_HOOK_OFFSETLESS(ScoreController_Start, void, ScoreController* self) {
    ScoreController_Start(self);
    self->add_noteWasCutEvent(il2cpp_utils::MakeDelegate<System::Action_3<NoteData*, NoteCutInfo*, int>*>(
        classof(System::Action_3<NoteData*, NoteCutInfo*, int>), self, +[](ScoreController* self, NoteData* data, NoteCutInfo* info, int unused) {
            //QounterRegistry::BroadcastEvent(QounterRegistry::Event::NoteCut, data, info);
        }
    ));
    self->add_noteWasMissedEvent(il2cpp_utils::MakeDelegate<System::Action_2<NoteData*, int>*>(
        classof(System::Action_2<NoteData*, int>*), self, +[](ScoreController* self, NoteData* data, int unused) {
            //QounterRegistry::BroadcastEvent(QounterRegistry::Event::NoteMiss, data);
        }
    ));
    self->add_scoreDidChangeEvent(il2cpp_utils::MakeDelegate<System::Action_2<int, int>*>(
        classof(System::Action_2<int, int>*), self, +[](ScoreController* self, int rawScore, int modifiedScore) {
            //QounterRegistry::BroadcastEvent(QounterRegistry::Event::ScoreUpdated, modifiedScore);
        }
    ));
    self->add_immediateMaxPossibleScoreDidChangeEvent(il2cpp_utils::MakeDelegate<System::Action_2<int, int>*>(
        classof(System::Action_2<int, int>*), self, +[](ScoreController* self, int rawScore, int modifiedScore) {

            //QounterRegistry::BroadcastEvent(QounterRegistry::Event::MaxScoreUpdated, modifiedScore);
        }
    ));
}
*//*
    const MethodInfo* addCompletedMethod = CRASH_UNLESS(il2cpp_utils::FindMethodUnsafe(audioClipAsync, "add_completed", 1));
    auto action = CRASH_UNLESS(il2cpp_utils::MakeDelegate(addCompletedMethod, 0, (Il2CppObject*)this, audioClipCompleted));
    CRASH_UNLESS(il2cpp_utils::RunMethod(audioClipAsync, addCompletedMethod, action));
*/

/*
const std::vector<QounterRegistry::EventHandlerSignature> QounterRegistry::eventHandlerSignatures = {
    {QounterRegistry::Event::NoteCut, "OnNoteCut", 2},
    {QounterRegistry::Event::NoteMiss, "OnNoteMiss", 1},
    {QounterRegistry::Event::ScoreUpdated, "OnScoreUpdated", 1},
    {QounterRegistry::Event::MaxScoreUpdated, "OnMaxScoreUpdated", 1},
    {QounterRegistry::Event::SwingRatingFinished, "OnSwingRatingFinished", 2},
};
std::unordered_map<std::pair<std::string, std::string>, QounterRegistry::RegistryEntry, pair_hash> QounterRegistry::registry;
std::vector<std::pair<std::string, std::string>> QounterRegistry::registryInsertionOrder;
*/

// Multiplayer song starting is handled differently
MAKE_HOOK_OFFSETLESS(MultiplayerSongStart, void, Il2CppObject* self, Il2CppString* gameMode, Il2CppObject* previewBeatmapLevel, int beatmapDifficulty, Il2CppObject* a, Il2CppObject* b, Il2CppObject* c, Il2CppObject* d, Il2CppObject* e, Il2CppObject* f, bool g) {
    getLogger().info("Multiplayer Song Started");
    selectedLevel.selectedDifficulty = difficultyToString(beatmapDifficulty);
    presenceManager->statusLock.lock();
    presenceManager->playingLevel.emplace(selectedLevel);
    presenceManager->statusLock.unlock();


    MultiplayerSongStart(self, gameMode, previewBeatmapLevel, beatmapDifficulty, a, b, c, d, e, f, g);
}

void onPlayerJoin() {
    presenceManager->statusLock.lock();
    presenceManager->multiplayerLobby->numberOfPlayers++;
    presenceManager->statusLock.unlock();
}

void onPlayerLeave() {
    presenceManager->statusLock.lock();
    presenceManager->multiplayerLobby->numberOfPlayers--;
    presenceManager->statusLock.unlock();
}

// Reset the lobby back to null when we leave back to the menu
void onLobbyDisconnect() {
    getLogger().info("Left Multiplayer lobby");
    presenceManager->statusLock.lock();
    presenceManager->multiplayerLobby = std::nullopt;
    presenceManager->statusLock.unlock();
}

MAKE_HOOK_OFFSETLESS(MultiplayerJoinLobby, void, GameServerLobbyFlowCoordinator* self, bool firstActivation, bool addedToHierarchy, bool screenSystemEnabling)    {
    getLogger().info("Joined multiplayer lobby");
    IMultiplayerSessionManager* sessionManager = self->multiplayerSessionManager;

    int maxPlayers = sessionManager->get_maxPlayerCount();
    int numActivePlayers = sessionManager->get_connectedPlayerCount();

    // Register player join and leave events
    sessionManager->add_playerDisconnectedEvent(
        il2cpp_utils::MakeDelegate<System::Action_1<IConnectedPlayer*>*>(classof(System::Action_1<IConnectedPlayer*>*), static_cast<Il2CppObject*>(nullptr), onPlayerLeave)
    );

    sessionManager->add_playerConnectedEvent(
        il2cpp_utils::MakeDelegate<System::Action_1<IConnectedPlayer*>*>(classof(System::Action_1<IConnectedPlayer*>*), static_cast<Il2CppObject*>(nullptr), onPlayerJoin)
    );

    // Register disconnect from lobby event
    sessionManager->add_disconnectedEvent(
        il2cpp_utils::MakeDelegate<System::Action_1<GlobalNamespace::DisconnectedReason>*>(classof(System::Action_1<GlobalNamespace::DisconnectedReason>*), static_cast<Il2CppObject*>(nullptr), onLobbyDisconnect)
    );

    // Set the number of players in this lobby
    MultiplayerLobbyInfo lobbyInfo;
    lobbyInfo.numberOfPlayers = numActivePlayers + 1;
    lobbyInfo.maxPlayers = maxPlayers;
    presenceManager->statusLock.lock();
    presenceManager->multiplayerLobby.emplace(lobbyInfo);
    presenceManager->statusLock.unlock();

    MultiplayerJoinLobby(self, firstActivation, addedToHierarchy, screenSystemEnabling);
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
    presenceManager->time = (int)time;
    presenceManager->endTime = (int)endTime;
    presenceManager->statusLock.unlock();
}

// TODO: Replace the config code, and do stuff internally
void saveDefaultConfig()  {
    getLogger().info("Creating config file . . .");
    ConfigDocument& config = getConfig().config;
    auto& alloc = config.GetAllocator();
    // If the config has already been created, don't overwrite it
    //if(config.HasMember("multiplayerLevelPresence")) {
    //    getLogger().info("Config file already exists");
    //    return;
    //}
    config.RemoveAllMembers();
    config.SetObject();
    // Create the sections of the config file for each type of presence
    rapidjson::Value levelPresence(rapidjson::kObjectType);
    levelPresence.AddMember("details", "{mapName}", alloc);
    levelPresence.AddMember("mapSubName", "{mapSubName}", alloc);
    levelPresence.AddMember("mapDifficulty", "{mapDifficulty}", alloc);
    levelPresence.AddMember("mapAuthor",  "{mapAuthor}", alloc);
    levelPresence.AddMember("songAuthor", "{songAuthor}", alloc);
    levelPresence.AddMember("levelID", "{levelID}", alloc);
    levelPresence.AddMember("players", "{numPlayers}/{maxPlayers}", alloc);
    levelPresence.AddMember("state", "{paused?}", alloc);
    config.AddMember("standardLevelPresence", levelPresence, alloc);

    rapidjson::Value practicePresence(rapidjson::kObjectType);
    practicePresence.AddMember("details", "{mapName}", alloc);
    practicePresence.AddMember("mapSubName", "{mapSubName}", alloc);
    practicePresence.AddMember("mapDifficulty", "{mapDifficulty}", alloc);
    practicePresence.AddMember("mapAuthor", "{mapAuthor}", alloc);
    practicePresence.AddMember("songAuthor", "{songAuthor}", alloc);
    practicePresence.AddMember("levelID", "{levelID}", alloc);
    practicePresence.AddMember("players", "{numPlayers}/{maxPlayers}", alloc);
    practicePresence.AddMember("state",  "{paused?}", alloc);
    config.AddMember("practicePresence", practicePresence, alloc);

    rapidjson::Value multiLevelPresence(rapidjson::kObjectType);
    multiLevelPresence.AddMember("details", "{mapName}", alloc);
    multiLevelPresence.AddMember("mapSubName", "{mapSubName}", alloc);
    multiLevelPresence.AddMember("mapDifficulty", "{mapDifficulty}", alloc);
    multiLevelPresence.AddMember("mapAuthor", "{mapAuthor}", alloc);
    multiLevelPresence.AddMember("songAuthor", "{songAuthor}", alloc);
    multiLevelPresence.AddMember("levelID", "{levelID}", alloc);
    multiLevelPresence.AddMember("players", "{numPlayers}/{maxPlayers}", alloc);
    multiLevelPresence.AddMember("state",  "{paused?}", alloc);
    config.AddMember("multiplayerLevelPresence", multiLevelPresence, alloc);

    rapidjson::Value missionPresence(rapidjson::kObjectType);
    missionPresence.AddMember("details", "Playing Campaign", alloc);
    missionPresence.AddMember("mapSubName", "", alloc);
    missionPresence.AddMember("mapDifficulty", "", alloc);
    missionPresence.AddMember("mapAuthor", "", alloc);
    missionPresence.AddMember("songAuthor", "", alloc);
    missionPresence.AddMember("levelID", "", alloc);
    missionPresence.AddMember("players", "", alloc);
    missionPresence.AddMember("state",  "{paused?}", alloc);
    config.AddMember("missionLevelPresence", missionPresence, alloc);

    rapidjson::Value tutorialPresence(rapidjson::kObjectType);
    tutorialPresence.AddMember("details", "Playing Tutorial", alloc);
    tutorialPresence.AddMember("mapSubName", "", alloc);
    tutorialPresence.AddMember("mapDifficulty", "", alloc);
    tutorialPresence.AddMember("mapAuthor", "", alloc);
    tutorialPresence.AddMember("songAuthor", "", alloc);
    tutorialPresence.AddMember("levelID", "", alloc);
    tutorialPresence.AddMember("players", "", alloc);
    tutorialPresence.AddMember("state",  "{paused?}", alloc);
    config.AddMember("tutorialPresence", tutorialPresence, alloc);

    rapidjson::Value multiLobbyPresence(rapidjson::kObjectType);
    multiLobbyPresence.AddMember("details", "Multiplayer - In Lobby", alloc);
    multiLobbyPresence.AddMember("mapSubName", "", alloc);
    multiLobbyPresence.AddMember("mapDifficulty", "", alloc);
    multiLobbyPresence.AddMember("mapAuthor", "", alloc);
    multiLobbyPresence.AddMember("songAuthor", "", alloc);
    multiLobbyPresence.AddMember("levelID", "", alloc);
    multiLobbyPresence.AddMember("players", "{numPlayers}/{maxPlayers}", alloc);
    multiLobbyPresence.AddMember("state", "", alloc);
    config.AddMember("multiplayerLobbyPresence", multiLobbyPresence, alloc);

    rapidjson::Value menuPresence(rapidjson::kObjectType);
    menuPresence.AddMember("details", "In Menu", alloc);
    menuPresence.AddMember("mapSubName", "", alloc);
    menuPresence.AddMember("mapDifficulty", "", alloc);
    menuPresence.AddMember("mapAuthor", "", alloc);
    menuPresence.AddMember("songAuthor", "", alloc);
    menuPresence.AddMember("levelID", "", alloc);
    menuPresence.AddMember("players", "", alloc);
    menuPresence.AddMember("state",  "", alloc);
    config.AddMember("menuPresence", menuPresence, alloc);

    getConfig().Write();
    getLogger().info("Config file created");
}

extern "C" void setup(ModInfo& info) {
    info.id = "streamer-tools";
    info.version = "0.1.0";
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
    Logger& logger = getLogger();
    INSTALL_HOOK_OFFSETLESS(logger, RefreshContent, il2cpp_utils::FindMethodUnsafe("", "StandardLevelDetailView", "RefreshContent", 0));
    INSTALL_HOOK_OFFSETLESS(logger, SongStart, il2cpp_utils::FindMethodUnsafe("", "StandardLevelScenesTransitionSetupDataSO", "Init", 9));
    INSTALL_HOOK_OFFSETLESS(logger, SongEnd, il2cpp_utils::FindMethodUnsafe("", "StandardLevelGameplayManager", "OnDestroy", 0));
    INSTALL_HOOK_OFFSETLESS(logger, CampaignLevelStart, il2cpp_utils::FindMethodUnsafe("", "MissionLevelScenesTransitionSetupDataSO", "Init", 7));
    INSTALL_HOOK_OFFSETLESS(logger, CampaignLevelEnd, il2cpp_utils::FindMethodUnsafe("", "MissionLevelGameplayManager", "OnDestroy", 0));
    INSTALL_HOOK_OFFSETLESS(logger, TutorialStart, il2cpp_utils::FindMethodUnsafe("", "TutorialSongController", "Awake", 0));
    INSTALL_HOOK_OFFSETLESS(logger, TutorialEnd, il2cpp_utils::FindMethodUnsafe("", "TutorialSongController", "OnDestroy", 0));
    INSTALL_HOOK_OFFSETLESS(logger, GamePause, il2cpp_utils::FindMethodUnsafe("", "PauseController", "Pause", 0));
    INSTALL_HOOK_OFFSETLESS(logger, GameResume, il2cpp_utils::FindMethodUnsafe("", "PauseController", "HandlePauseMenuManagerDidPressContinueButton", 0));
    INSTALL_HOOK_OFFSETLESS(logger, AudioUpdate, il2cpp_utils::FindMethodUnsafe("", "AudioTimeSyncController", "Update", 0));
    INSTALL_HOOK_OFFSETLESS(logger, MultiplayerSongStart, il2cpp_utils::FindMethodUnsafe("", "MultiplayerLevelScenesTransitionSetupDataSO", "Init", 10));
    INSTALL_HOOK_OFFSETLESS(logger, MultiplayerJoinLobby, il2cpp_utils::FindMethodUnsafe("", "GameServerLobbyFlowCoordinator", "DidActivate", 3));
    INSTALL_HOOK_OFFSETLESS(logger, MultiplayerSongEnd, il2cpp_utils::FindMethodUnsafe("", "MultiplayerLocalActivePlayerGameplayManager", "OnDisable", 0));
    INSTALL_HOOK_OFFSETLESS(logger, ScoreController_Start, il2cpp_utils::FindMethodUnsafe("", "ScoreController", "Start", 0));

    getLogger().debug("Installed all hooks!");
    presenceManager = new PresenceManager(getLogger(), getConfig().config);
}
