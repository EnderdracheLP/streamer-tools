#include "beatsaber-hook/shared/utils/typedefs.h"
#include "beatsaber-hook/shared/utils/il2cpp-functions.hpp"
#include "beatsaber-hook/shared/utils/utils.h"
#include "beatsaber-hook/shared/utils/logging.hpp"
#include "beatsaber-hook/shared/utils/il2cpp-utils.hpp"
#include "beatsaber-hook/shared/utils/typedefs.h"
#include "beatsaber-hook/shared/config/config-utils.hpp"

#include "UnityEngine/Resources.hpp"
#include "UnityEngine/GameObject.hpp"
#include "UnityEngine/Component.hpp"

#include "UnityEngine/Sprite.hpp"
#include "UnityEngine/Texture2D.hpp"

#include "System/Action_1.hpp"
#include "System/Action_2.hpp"
#include "System/Action_3.hpp"

#include "System/Func_2.hpp"

#include "System/Threading/Tasks/Task_1.hpp"
#include "System/Threading/CancellationToken.hpp"

#include "System/Convert.hpp"

#include "GlobalNamespace/IConnectedPlayer.hpp"
#include "GlobalNamespace/MultiplayerPlayersManager.hpp"
#include "GlobalNamespace/MultiplayerSessionManager.hpp"
#include "GlobalNamespace/GameServerLobbyFlowCoordinator.hpp"
#include "GlobalNamespace/PracticeSettings.hpp"
#include "GlobalNamespace/ScoreController.hpp"  // Added for getting Score Information.
#include "GlobalNamespace/RelativeScoreAndImmediateRankCounter.hpp"
#include "GlobalNamespace/ScoreController.hpp"
#include "GlobalNamespace/GameEnergyUIPanel.hpp"
#include "GlobalNamespace/StandardLevelDetailView.hpp"
#include "GlobalNamespace/IDifficultyBeatmap.hpp"
#include "GlobalNamespace/IBeatmapLevel.hpp"
#include "GlobalNamespace/MultiplayerSettingsPanelController.hpp"
#include "GlobalNamespace/ServerCodeView.hpp"
#include "GlobalNamespace/ScoreController.hpp"
#include "GlobalNamespace/NoteController.hpp"
#include "GlobalNamespace/NoteCutInfo.hpp"
#include "GlobalNamespace/FPSCounter.hpp"
#include "GlobalNamespace/GameplayCoreInstaller.hpp"

#include "UnityEngine/SceneManagement/Scene.hpp"
using namespace GlobalNamespace;

#include "modloader/shared/modloader.hpp"

#include <string>
#include <optional>
#include "STmanager.hpp"

// static ModInfo modInfo;
ModInfo STModInfo;
static Configuration& getConfig() {
    static Configuration config(STModInfo);
    return config;
}

static Logger& getLogger() {
    static Logger* logger = new Logger(STModInfo);
    return *logger;
}
static STManager* stManager = nullptr;


// Define the current level by finding info from the IBeatmapLevel object
MAKE_HOOK_OFFSETLESS(RefreshContent, void, StandardLevelDetailView* self) {
    RefreshContent(self);

    Il2CppObject* level = CRASH_UNLESS(il2cpp_utils::GetFieldValue(self, "_level"));
    // Check if the level is an instance of BeatmapLevelSO
    stManager->statusLock.lock();
    stManager->levelName = to_utf8(csstrtostr((Il2CppString*) CRASH_UNLESS(il2cpp_utils::GetPropertyValue(level, "songName"))));
    stManager->levelSubName = to_utf8(csstrtostr((Il2CppString*)CRASH_UNLESS(il2cpp_utils::GetPropertyValue(level, "songSubName"))));
    stManager->levelAuthor = to_utf8(csstrtostr((Il2CppString*) CRASH_UNLESS(il2cpp_utils::GetPropertyValue(level, "levelAuthorName"))));
    stManager->songAuthor = to_utf8(csstrtostr((Il2CppString*) CRASH_UNLESS(il2cpp_utils::GetPropertyValue(level, "songAuthorName"))));
    stManager->id = to_utf8(csstrtostr((Il2CppString*)CRASH_UNLESS(il2cpp_utils::GetPropertyValue(level, "levelID"))));
    stManager->bpm = CRASH_UNLESS(il2cpp_utils::GetPropertyValue<float>(level, "beatsPerMinute"));
    bool CustomLevel = stManager->id.find("custom_level_") != std::string::npos;
    stManager->njs = CustomLevel ? self->selectedDifficultyBeatmap->get_noteJumpMovementSpeed() : 0.0f;

    static Unity::Collections::NativeArray_1<uint8_t> rawCover;

    System::Threading::Tasks::Task_1<UnityEngine::Sprite*>*  _coverGetter = reinterpret_cast<GlobalNamespace::IPreviewBeatmapLevel*>(self->level)->GetCoverImageAsync(System::Threading::CancellationToken::get_None());
    System::Threading::Tasks::Task_1<Array<uint8_t>*>*  _rawCoverGetter = _coverGetter->ContinueWith<Array<uint8_t>*>(il2cpp_utils::MakeFunc<System::Func_2<System::Threading::Tasks::Task_1<UnityEngine::Sprite*>*, Array<uint8_t>*>*>(
        *[](System::Threading::Tasks::Task_1<UnityEngine::Sprite*>* spriteTask)->Array<uint8_t>*{
            UnityEngine::Sprite* sprite = spriteTask->get_Result();
            rawCover = sprite->get_texture()->GetRawTextureData<uint8_t>();
            return reinterpret_cast<Array<uint8_t>*>(&rawCover);
        }
    ));

    auto RawCoverbytesArray = Array<uint8_t*>::NewLength(rawCover.m_Length);
    memcpy(RawCoverbytesArray->values, rawCover.m_Buffer, rawCover.m_Length);

    stManager->coverImageBase64 = to_utf8(csstrtostr(System::Convert::ToBase64String(reinterpret_cast<Array<uint8_t>*>(RawCoverbytesArray))));

    //*/
    stManager->statusLock.unlock();
}

MAKE_HOOK_OFFSETLESS(SongStart, void, Il2CppObject* self, Il2CppString* gameMode, Il2CppObject* difficultyBeatmap, Il2CppObject* b, Il2CppObject* c, Il2CppObject* d, Il2CppObject* e, Il2CppObject* f, PracticeSettings* practiceSettings, Il2CppString* g, bool h) {
    getLogger().info("Song Started");
    int difficulty = CRASH_UNLESS(il2cpp_utils::GetPropertyValue<int>(difficultyBeatmap, "difficulty"));

    // Set the currently playing level to the selected one, since we are in a song
    stManager->statusLock.lock();
    stManager->difficulty = difficulty;
    stManager->goodCuts = 0;
    stManager->badCuts = 0;
    stManager->missedNotes = 0;
    stManager->isPractice = practiceSettings; // If practice settings isn't null, then we're in practice mode
    stManager->statusLock.unlock();
    SongStart(self, gameMode, difficultyBeatmap, b, c, d, e, f, practiceSettings, g, h);
}

MAKE_HOOK_OFFSETLESS(RelativeScoreAndImmediateRankCounter_UpdateRelativeScoreAndImmediateRank, void, RelativeScoreAndImmediateRankCounter* self, int score, int modifiedScore, int maxPossibleScore, int maxPossibleModifiedScore) {
    RelativeScoreAndImmediateRankCounter_UpdateRelativeScoreAndImmediateRank(self, score, modifiedScore, maxPossibleScore, maxPossibleModifiedScore);
    stManager->statusLock.lock();
    stManager->score = modifiedScore;
    stManager->rank = to_utf8(csstrtostr(RankModel::GetRankName(self->get_immediateRank())));
    stManager->accuracy = self->get_relativeScore();
    stManager->statusLock.unlock();
}

MAKE_HOOK_OFFSETLESS(GameEnergyUIPanel_HandleGameEnergyDidChange, void, GameEnergyUIPanel* self, float energy) {
    GameEnergyUIPanel_HandleGameEnergyDidChange(self, energy);
    stManager->statusLock.lock();
    stManager->energy = energy;
    stManager->statusLock.unlock();
}

MAKE_HOOK_OFFSETLESS(ServerCodeView_RefreshText, void, ServerCodeView* self, bool refreshText) {
    ServerCodeView_RefreshText(self, refreshText);
    stManager->statusLock.lock();
    stManager->mpGameId = to_utf8(csstrtostr(self->serverCode));
    stManager->mpGameIdShown = self->codeIsShown;
    stManager->statusLock.unlock();
}

MAKE_HOOK_OFFSETLESS(ScoreController_Update, void, ScoreController* self) {
    stManager->statusLock.lock();
    stManager->combo = self->combo;
    stManager->statusLock.unlock();
    ScoreController_Update(self);
}

// Multiplayer song starting is handled differently
MAKE_HOOK_OFFSETLESS(MultiplayerSongStart, void, Il2CppObject* self, Il2CppString* gameMode, Il2CppObject* previewBeatmapLevel, int beatmapDifficulty, Il2CppObject* a, Il2CppObject* b, Il2CppObject* c, Il2CppObject* d, Il2CppObject* e, Il2CppObject* f, bool g) {
    getLogger().info("Multiplayer Song Started");
    stManager->statusLock.lock();
    stManager->location = 2;
    stManager->goodCuts = 0;
    stManager->badCuts = 0;
    stManager->missedNotes = 0;
    stManager->statusLock.unlock();


    MultiplayerSongStart(self, gameMode, previewBeatmapLevel, beatmapDifficulty, a, b, c, d, e, f, g);
}

void onPlayerJoin() {
    stManager->statusLock.lock();
    stManager->players++;
    stManager->statusLock.unlock();
}

void onPlayerLeave() {
    stManager->statusLock.lock();
    stManager->players--;
    stManager->statusLock.unlock();
}

// Reset the lobby back to null when we leave back to the menu
void onLobbyDisconnect() {
    getLogger().info("Left Multiplayer lobby");
    stManager->statusLock.lock();
    stManager->location = 0;
    stManager->statusLock.unlock();
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
    stManager->statusLock.lock();
    stManager->players = numActivePlayers + 1;
    stManager->maxPlayers = maxPlayers;
    stManager->location = 5;
    stManager->statusLock.unlock();

    MultiplayerJoinLobby(self, firstActivation, addedToHierarchy, screenSystemEnabling);
}

MAKE_HOOK_OFFSETLESS(SongEnd, void, Il2CppObject* self) {
    getLogger().info("Song Ended");
    stManager->statusLock.lock();
    stManager->paused = false; // If we are pasued, unpause us, since we are returning to the menu
    stManager->location = 0;
    stManager->statusLock.unlock();
    SongEnd(self);
}

MAKE_HOOK_OFFSETLESS(MultiplayerSongEnd, void, Il2CppObject* self) {
    getLogger().info("Multiplayer Song Ended");
    stManager->statusLock.lock();
    stManager->location = 0;
    stManager->paused = false; // If we are pasued, unpause us, since we are returning to the menu
    stManager->statusLock.unlock();
    SongEnd(self);
}

MAKE_HOOK_OFFSETLESS(TutorialStart, void, Il2CppObject* self)   {
    getLogger().info("Tutorial starting");
    stManager->statusLock.lock();
    stManager->location = 3;
    stManager->goodCuts = 0;
    stManager->badCuts = 0;
    stManager->missedNotes = 0;
    stManager->statusLock.unlock();
    TutorialStart(self);
}
MAKE_HOOK_OFFSETLESS(TutorialEnd, void, Il2CppObject* self)   {
    getLogger().info("Tutorial ending");
    stManager->statusLock.lock();
    stManager->location = 0;
    stManager->paused = false; // If we are pasued, unpause us, since we are returning to the menu
    stManager->statusLock.unlock();
    TutorialEnd(self);
}

MAKE_HOOK_OFFSETLESS(CampaignLevelStart, void, Il2CppObject* self, Il2CppString* missionId, Il2CppObject* a, Il2CppArray* b, Il2CppObject* c, Il2CppObject* d, Il2CppObject* e, Il2CppObject* f, Il2CppString* g)   {
    getLogger().info("Campaign level starting");
    stManager->statusLock.lock();
    stManager->location = 4;
    stManager->goodCuts = 0;
    stManager->badCuts = 0;
    stManager->missedNotes = 0;
    stManager->statusLock.unlock();
    CampaignLevelStart(self, missionId, a, b, c, d, e, f, g);
}
MAKE_HOOK_OFFSETLESS(CampaignLevelEnd, void, Il2CppObject* self)   {
    getLogger().info("Campaign level ending");
    stManager->statusLock.lock();
    stManager->location = 0;
    stManager->paused = false; // If we are paused, unpause us, since we are returning to the menu
    stManager->statusLock.unlock();
    CampaignLevelEnd(self);
}

MAKE_HOOK_OFFSETLESS(GamePause, void, Il2CppObject* self)   {
    getLogger().info("Game paused");
    stManager->statusLock.lock();
    stManager->paused = true;
    stManager->statusLock.unlock();
    GamePause(self);
}
MAKE_HOOK_OFFSETLESS(GameResume, void, Il2CppObject* self)   {
    getLogger().info("Game resumed");
    stManager->statusLock.lock();
    stManager->paused = false;
    stManager->statusLock.unlock();
    GameResume(self);
}

MAKE_HOOK_OFFSETLESS(AudioUpdate, void, Il2CppObject* self) {
    AudioUpdate(self);

    float time = CRASH_UNLESS(il2cpp_utils::RunMethodUnsafe<float>(self, "get_songTime"));
    float endTime = CRASH_UNLESS(il2cpp_utils::RunMethodUnsafe<float>(self, "get_songEndTime"));

    stManager->statusLock.lock();
    stManager->time = (int)time;
    stManager->endTime = (int)endTime;
    stManager->statusLock.unlock();
}

MAKE_HOOK_OFFSETLESS(ScoreController_HandleNoteWasMissed, void, ScoreController* self, NoteController* note) {
    ScoreController_HandleNoteWasMissed(self, note);
    stManager->statusLock.lock();
    stManager->missedNotes++;
    stManager->statusLock.unlock();
}

MAKE_HOOK_OFFSETLESS(ScoreController_HandleNoteWasCut, void, ScoreController* self, NoteController* note, NoteCutInfo* info) {
    ScoreController_HandleNoteWasCut(self, note, info);
    stManager->statusLock.lock();
    if(info->get_allIsOK()) stManager->goodCuts++;
    else stManager->badCuts++;
    stManager->statusLock.unlock();
}

MAKE_HOOK_OFFSETLESS(FPSCounter_Update, void, FPSCounter* self) {
    FPSCounter_Update(self);
    
    stManager->statusLock.lock();
    stManager->fps = self->get_currentFPS();
    stManager->statusLock.unlock();
}

bool FPSObjectCreated = false;

MAKE_HOOK_OFFSETLESS(SceneManager_ActiveSceneChanged, void, UnityEngine::SceneManagement::Scene previousActiveScene, UnityEngine::SceneManagement::Scene nextActiveScene) {
    
    if (nextActiveScene.IsValid()) {
        std::string sceneName = to_utf8(csstrtostr(nextActiveScene.get_name()));
        std::string shaderWarmup = "ShaderWarmup";
        if (sceneName == shaderWarmup) {
            auto FPSCObject = UnityEngine::GameObject::New_ctor(il2cpp_utils::newcsstr("FPSC"));
            UnityEngine::Object::DontDestroyOnLoad(FPSCObject->AddComponent<FPSCounter*>());
        }
        FPSObjectCreated = true;
    }

    SceneManager_ActiveSceneChanged(previousActiveScene, nextActiveScene);
}

extern "C" void setup(ModInfo& info) {
    info.id = ID;
    info.version = VERSION;
    STModInfo = info;
    getLogger().info("Modloader name: %s", Modloader::getInfo().name.c_str());

    getLogger().info("Completed setup!");
}

extern "C" void load() {
    getLogger().debug("Installing hooks...");
    il2cpp_functions::Init();

    // Install our function hooks
    Logger& logger = getLogger();
    INSTALL_HOOK_OFFSETLESS(logger, RefreshContent, il2cpp_utils::FindMethodUnsafe("", "StandardLevelDetailView", "RefreshContent", 0));
    INSTALL_HOOK_OFFSETLESS(logger, SongStart, il2cpp_utils::FindMethodUnsafe("", "StandardLevelScenesTransitionSetupDataSO", "Init", 10));
    INSTALL_HOOK_OFFSETLESS(logger, RelativeScoreAndImmediateRankCounter_UpdateRelativeScoreAndImmediateRank, il2cpp_utils::FindMethodUnsafe("", "RelativeScoreAndImmediateRankCounter", "UpdateRelativeScoreAndImmediateRank", 4));
    INSTALL_HOOK_OFFSETLESS(logger, ScoreController_Update, il2cpp_utils::FindMethodUnsafe("", "ScoreController", "Update", 0));
    INSTALL_HOOK_OFFSETLESS(logger, SongEnd, il2cpp_utils::FindMethodUnsafe("", "StandardLevelGameplayManager", "OnDestroy", 0));
    INSTALL_HOOK_OFFSETLESS(logger, CampaignLevelStart, il2cpp_utils::FindMethodUnsafe("", "MissionLevelScenesTransitionSetupDataSO", "Init", 8));
    INSTALL_HOOK_OFFSETLESS(logger, CampaignLevelEnd, il2cpp_utils::FindMethodUnsafe("", "MissionLevelGameplayManager", "OnDestroy", 0));
    INSTALL_HOOK_OFFSETLESS(logger, TutorialStart, il2cpp_utils::FindMethodUnsafe("", "TutorialSongController", "Awake", 0));
    INSTALL_HOOK_OFFSETLESS(logger, TutorialEnd, il2cpp_utils::FindMethodUnsafe("", "TutorialSongController", "OnDestroy", 0));
    INSTALL_HOOK_OFFSETLESS(logger, GamePause, il2cpp_utils::FindMethodUnsafe("", "PauseController", "Pause", 0));
    INSTALL_HOOK_OFFSETLESS(logger, GameResume, il2cpp_utils::FindMethodUnsafe("", "PauseController", "HandlePauseMenuManagerDidPressContinueButton", 0));
    INSTALL_HOOK_OFFSETLESS(logger, AudioUpdate, il2cpp_utils::FindMethodUnsafe("", "AudioTimeSyncController", "Update", 0));
    INSTALL_HOOK_OFFSETLESS(logger, MultiplayerSongStart, il2cpp_utils::FindMethodUnsafe("", "MultiplayerLevelScenesTransitionSetupDataSO", "Init", 10));
    INSTALL_HOOK_OFFSETLESS(logger, MultiplayerJoinLobby, il2cpp_utils::FindMethodUnsafe("", "GameServerLobbyFlowCoordinator", "DidActivate", 3));
    INSTALL_HOOK_OFFSETLESS(logger, MultiplayerSongEnd, il2cpp_utils::FindMethodUnsafe("", "MultiplayerLocalActivePlayerGameplayManager", "OnDisable", 0));
    INSTALL_HOOK_OFFSETLESS(logger, GameEnergyUIPanel_HandleGameEnergyDidChange, il2cpp_utils::FindMethodUnsafe("", "GameEnergyUIPanel", "HandleGameEnergyDidChange", 1));
    INSTALL_HOOK_OFFSETLESS(logger, ServerCodeView_RefreshText, il2cpp_utils::FindMethodUnsafe("", "ServerCodeView", "RefreshText", 1));
    INSTALL_HOOK_OFFSETLESS(logger, ScoreController_HandleNoteWasMissed, il2cpp_utils::FindMethodUnsafe("", "ScoreController", "HandleNoteWasMissed", 1));
    INSTALL_HOOK_OFFSETLESS(logger, ScoreController_HandleNoteWasCut, il2cpp_utils::FindMethodUnsafe("", "ScoreController", "HandleNoteWasCut", 2));
    INSTALL_HOOK_OFFSETLESS(logger, FPSCounter_Update, il2cpp_utils::FindMethodUnsafe("", "FPSCounter", "Update", 0));
    INSTALL_HOOK_OFFSETLESS(logger, SceneManager_ActiveSceneChanged, il2cpp_utils::FindMethodUnsafe("UnityEngine.SceneManagement", "SceneManager", "Internal_ActiveSceneChanged", 2));

    getLogger().debug("Installed all hooks!");
    stManager = new STManager(getLogger(), getConfig().config);
}