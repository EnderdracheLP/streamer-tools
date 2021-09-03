#include "modloader/shared/modloader.hpp"
#include "STmanager.hpp"
#include "SettingsViewController.hpp"
#include "Config.hpp"

#include "custom-types/shared/register.hpp"

#include "beatsaber-hook/shared/utils/typedefs.h"
#include "beatsaber-hook/shared/utils/il2cpp-functions.hpp"
#include "beatsaber-hook/shared/utils/utils.h"
#include "beatsaber-hook/shared/utils/logging.hpp"
#include "beatsaber-hook/shared/utils/il2cpp-utils.hpp"
#include "beatsaber-hook/shared/utils/typedefs.h"
#ifndef MAKE_HOOK_OFFSETLESS
#include "beatsaber-hook/shared/utils/hooking.hpp"
#endif
//#include "beatsaber-hook/shared/config/config-utils.hpp"

#include "UnityEngine/Resources.hpp"
#include "UnityEngine/GameObject.hpp"
#include "UnityEngine/Component.hpp"
#include "UnityEngine/SceneManagement/Scene.hpp"
#include "UnityEngine/SceneManagement/SceneManager.hpp"
#include "UnityEngine/Application.hpp"

#include "UnityEngine/ImageConversion.hpp"
#include "UnityEngine/Sprite.hpp"
#include "UnityEngine/RenderTexture.hpp"
#include "UnityEngine/Graphics.hpp"
#include "UnityEngine/Texture2D.hpp"
#include "UnityEngine/Rect.hpp"

#include "System/Action_1.hpp"
#include "System/Action_2.hpp"
#include "System/Action_3.hpp"
#include "System/Threading/Tasks/Task_1.hpp"
#include "System/Threading/Tasks/TaskStatus.hpp"
//#include "System/Func_2.hpp"

//#include "System/Threading/CancellationTokenSource.hpp"

#include "GlobalNamespace/IConnectedPlayer.hpp"
#include "GlobalNamespace/MultiplayerPlayersManager.hpp"
#include "GlobalNamespace/MultiplayerSessionManager.hpp"

#if defined(BS__1_13_2) || defined(BS__1_16) && BS__1_16 < 4
#include "GlobalNamespace/GameServerLobbyFlowCoordinator.hpp"
#else
#include "GlobalNamespace/MultiplayerLobbyConnectionController.hpp"
#endif

#include "GlobalNamespace/PracticeSettings.hpp"
#include "GlobalNamespace/ScoreController.hpp"
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
#include "GlobalNamespace/BeatmapDifficulty.hpp"
#include "GlobalNamespace/OVRPlugin.hpp"
#include "GlobalNamespace/OVRPlugin_SystemHeadset.hpp"
#include "GlobalNamespace/PreviewBeatmapLevelSO.hpp"
#include "GlobalNamespace/CustomPreviewBeatmapLevel.hpp"
#include "GlobalNamespace/MainMenuViewController.hpp"
#include "GlobalNamespace/OptionsViewController.hpp"
#include "GlobalNamespace/PlayerTransforms.hpp"
#include "GlobalNamespace/StandardLevelScenesTransitionSetupDataSO.hpp"
#include "GlobalNamespace/MissionLevelScenesTransitionSetupDataSO.hpp"
#include "GlobalNamespace/MultiplayerLevelScenesTransitionSetupDataSO.hpp"
#include "GlobalNamespace/StandardLevelGameplayManager.hpp"
#include "GlobalNamespace/MultiplayerLocalActivePlayerGameplayManager.hpp"
#include "GlobalNamespace/TutorialSongController.hpp"
#include "GlobalNamespace/MissionLevelGameplayManager.hpp"
#include "GlobalNamespace/PauseController.hpp"
#include "GlobalNamespace/AudioTimeSyncController.hpp"
using namespace GlobalNamespace;

#if defined(MAKE_HOOK_OFFSETLESS) && !defined(MAKE_HOOK_MATCH)
#define ST_MAKE_HOOK(name, mPtr, retval, ...) MAKE_HOOK_OFFSETLESS(name, retval, __VA_ARGS__)
#define ST_INSTALL_HOOK(logger, name, methodInfo) INSTALL_HOOK_OFFSETLESS(logger, name, methodInfo)
#elif defined(MAKE_HOOK_MATCH)
#define ST_MAKE_HOOK(name, mPtr, retval, ...) MAKE_HOOK_MATCH(name, mPtr, retval, __VA_ARGS__)
#define ST_INSTALL_HOOK(logger, name, methodInfo) INSTALL_HOOK(logger, name)
#else
#error No Compatible HOOK macro found
#endif
#if !defined(BS__1_13_2) && !defined(BS__1_16)
#define BS__1_17 0
#endif

//#define DEBUG_BUILD 1

DEFINE_CONFIG(ModConfig);

ModInfo STModInfo;

Logger& getLogger() {
    static auto logger = new Logger(STModInfo, LoggerOptions(false, true)); // Set first bool to true to silence logger, second bool defines output to file
    return *logger;
}

static STManager* stManager = nullptr;

void ResetScores() {
    stManager->goodCuts = 0;
    stManager->badCuts = 0;
    stManager->missedNotes = 0;
    stManager->combo = 0;
    stManager->score = 0;
    stManager->accuracy = 1.0f;
    stManager->energy = 0.5f;
}

UnityEngine::Texture2D* DuplicateTexture(UnityEngine::Texture2D* source) {
    using namespace UnityEngine;
    RenderTexture* renderTex = RenderTexture::GetTemporary(source->get_width(), source->get_height(), 0, UnityEngine::RenderTextureFormat::Default, UnityEngine::RenderTextureReadWrite::Linear);
    Graphics::Blit(source, renderTex);
    RenderTexture* previous = RenderTexture::get_active();
    RenderTexture::set_active(renderTex);
    Texture2D* readableText = Texture2D::New_ctor(source->get_width(), source->get_height());
    readableText->ReadPixels(UnityEngine::Rect(0, 0, renderTex->get_width(), renderTex->get_height()), 0, 0);
    readableText->Apply();
    RenderTexture::set_active(previous);
    RenderTexture::ReleaseTemporary(renderTex);
    return readableText;
}

bool CoverChanged[4] = { false };
int CoverStatus = Init;

void GetCoverTexture(System::Threading::Tasks::Task_1<UnityEngine::Sprite*>* coverSpriteTask) {
    using namespace System::Threading;
    using namespace UnityEngine;
    if (coverSpriteTask->get_IsCompleted()) {
        Sprite* coverSprite;
        coverSprite = coverSpriteTask->get_Result();
        UnityEngine::Texture2D* coverTexture;
        // Check if the Texture is Readable and if not duplicate it and read from that
        if (coverSprite->get_texture()->get_isReadable()) {
            coverTexture = coverSprite->get_texture();
        }
        else {
            stManager->cv.notify_one();
            CoverStatus = Failed;
            return;
        }
        stManager->statusLock.lock();
        stManager->coverTexture = coverTexture;
        stManager->statusLock.unlock();
        for (int i = 0; i < 4; i++) {
            CoverChanged[i] = true;
        }
        coverSpriteTask->Dispose();
        stManager->cv.notify_one();
        CoverStatus = Completed;
        getLogger().info("Successfully loaded CoverImage");
        stManager->coverFetchable = true;
    }
    else if (coverSpriteTask->get_IsFaulted()) {
        getLogger().error("GetCover Task Faulted: Satus is, %d", (int)coverSpriteTask->get_Status());
        stManager->cv.notify_one();
        CoverStatus = Failed;
        coverSpriteTask->Dispose();
    }
    else {
        getLogger().error("Task Error: Status is %d", (int)coverSpriteTask->get_Status());
        stManager->cv.notify_one();
        CoverStatus = Failed;
    }
}

// TODO: Put the below in a lambda for the task
// Haven't figured out lambdas for tasks continuation yet
void GetCover(PreviewBeatmapLevelSO* level) {
    using namespace System::Threading;
    using namespace UnityEngine;
    Tasks::Task_1<UnityEngine::Sprite*>* coverSpriteTask;
    getLogger().debug("CoverSpriteTask");
    int Attempts = 0;
GetCoverTask:
    CoverStatus = Running;
    std::lock_guard<std::mutex> lk(stManager->CoverLock);

    coverSpriteTask = level->GetCoverImageAsync(CancellationToken::get_None());
    bool CustomLevel = (il2cpp_functions::class_is_assignable_from(classof(CustomPreviewBeatmapLevel*), il2cpp_functions::object_get_class(reinterpret_cast<Il2CppObject*>(level))));
    if (CustomLevel) {
        auto action = il2cpp_utils::MakeDelegate<System::Action_1<System::Threading::Tasks::Task*>*>(classof(System::Action_1<System::Threading::Tasks::Task*>*), coverSpriteTask, GetCoverTexture);
        reinterpret_cast<System::Threading::Tasks::Task*>(coverSpriteTask)->ContinueWith(action);
    }
    else {
        if (coverSpriteTask->get_IsFaulted() && Attempts < 5) {
            coverSpriteTask->Dispose();
            Attempts++;
            goto GetCoverTask;
        }
        else if (coverSpriteTask->get_Status().value == 1) {
            getLogger().critical("Task queued, cannot wait cause it would result in the MainThread freezing! Skipping Task");
            stManager->cv.notify_one();
            CoverStatus = Failed;
            return;
        }
        if (coverSpriteTask->get_IsCompleted()) {
            Sprite* coverSprite;
            coverSprite = coverSpriteTask->get_Result();
            UnityEngine::Texture2D* coverTexture;
            // Check if the Texture is Readable and if not duplicate it and read from that
            if (coverSprite->get_texture()->get_isReadable()) {
                
                coverTexture = coverSprite->get_texture();
            }
            else {
                coverTexture = DuplicateTexture(coverSprite->get_texture());
            }
            stManager->coverTexture = coverTexture;
            for (int i = 0; i < 4; i++) {
                CoverChanged[i] = true;
            }
            coverSpriteTask->Dispose();
            stManager->cv.notify_one();
            CoverStatus = Completed;
            getLogger().info("Successfully loaded CoverImage");
        }
        else {
            stManager->cv.notify_one();
            CoverStatus = Failed;
            getLogger().error("Task Failed to load CoverImage: Status was: %d", coverSpriteTask->get_Status().value);
            if (coverSpriteTask->get_Status().value != 1) coverSpriteTask->Dispose();
        }
    }
}


ST_MAKE_HOOK(RefreshContent, &StandardLevelDetailView::RefreshContent, void, StandardLevelDetailView* self) {
    RefreshContent(self);

    // Null Check Level before trying to get any data
    if (self->level) {
        stManager->statusLock.lock();
        stManager->levelName = to_utf8(csstrtostr((Il2CppString*)CRASH_UNLESS(il2cpp_utils::GetPropertyValue(self->level, "songName"))));
        stManager->levelSubName = to_utf8(csstrtostr((Il2CppString*)CRASH_UNLESS(il2cpp_utils::GetPropertyValue(self->level, "songSubName"))));
        stManager->levelAuthor = to_utf8(csstrtostr((Il2CppString*)CRASH_UNLESS(il2cpp_utils::GetPropertyValue(self->level, "levelAuthorName"))));
        stManager->songAuthor = to_utf8(csstrtostr((Il2CppString*)CRASH_UNLESS(il2cpp_utils::GetPropertyValue(self->level, "songAuthorName"))));
        stManager->id = to_utf8(csstrtostr((Il2CppString*)CRASH_UNLESS(il2cpp_utils::GetPropertyValue(self->level, "levelID"))));
        stManager->bpm = CRASH_UNLESS(il2cpp_utils::GetPropertyValue<float>(self->level, "beatsPerMinute"));
        // Check if level can be assigned as CustomPreviewBeatmapLevel
        bool CustomLevel = (il2cpp_functions::class_is_assignable_from(classof(CustomPreviewBeatmapLevel*), il2cpp_functions::object_get_class(reinterpret_cast<Il2CppObject*>(self->level))));
        stManager->njs = self->selectedDifficultyBeatmap->get_noteJumpMovementSpeed();
        stManager->difficulty = self->selectedDifficultyBeatmap->get_difficulty().value;
        stManager->coverFetchable = false;
        GetCover(reinterpret_cast<GlobalNamespace::PreviewBeatmapLevelSO*>(self->level));
        stManager->statusLock.unlock();
    }
    else getLogger().info("BeatmapLevelSO is nullptr");
}
#if defined(BS__1_16) || defined(BS__1_17)
#define SONGSTARTHOOK ST_MAKE_HOOK(SongStart, &StandardLevelScenesTransitionSetupDataSO::Init, void, StandardLevelScenesTransitionSetupDataSO* self, Il2CppString* gameMode, IDifficultyBeatmap* difficultyBeatmap, IPreviewBeatmapLevel* previewBeatmapLevel, OverrideEnvironmentSettings* overrideEnvironmentSettings, ColorScheme* overrideColorScheme, GameplayModifiers* gameplayModifiers, PlayerSpecificSettings* playerSpecificSettings, PracticeSettings* practiceSettings, Il2CppString* backButtonText, bool useTestNoteCutSoundEffects)
#define SONGSTART SongStart(self, gameMode, difficultyBeatmap, previewBeatmapLevel, overrideEnvironmentSettings, overrideColorScheme, gameplayModifiers, playerSpecificSettings, practiceSettings, backButtonText, useTestNoteCutSoundEffects)
#define CAMPAIGNLEVELSTARTHOOK ST_MAKE_HOOK(CampaignLevelStart, &MissionLevelScenesTransitionSetupDataSO::Init, void, MissionLevelScenesTransitionSetupDataSO* self, Il2CppString* missionId, IDifficultyBeatmap* difficultyBeatmap, IPreviewBeatmapLevel* previewBeatmapLevel, Array<MissionObjective*>* missionObjectives, ColorScheme* overrideColorScheme, GameplayModifiers* gameplayModifiers, PlayerSpecificSettings* playerSpecificSettings, Il2CppString* backButtonText)
#define CAMPAIGNLEVELSTART CampaignLevelStart(self, missionId, difficultyBeatmap, previewBeatmapLevel, missionObjectives, overrideColorScheme, gameplayModifiers, playerSpecificSettings, backButtonText)
#elif defined(BS__1_13_2)
#define SONGSTARTHOOK MAKE_HOOK_OFFSETLESS(SongStart, void, Il2CppObject* self, Il2CppString* gameMode, Il2CppObject* difficultyBeatmap, IPreviewBeatmapLevel* previewBeatmapLevel, Il2CppObject* c, Il2CppObject* d, Il2CppObject* e, PracticeSettings* practiceSettings, Il2CppString* g, bool h)
#define CAMPAIGNLEVELSTARTHOOK MAKE_HOOK_OFFSETLESS(CampaignLevelStart, void, Il2CppObject* self, Il2CppObject* a, Il2CppArray* b, Il2CppObject* c, Il2CppObject* d, Il2CppObject* e, Il2CppObject* f, Il2CppString* g)
#define SONGSTART SongStart(self, gameMode, difficultyBeatmap, previewBeatmapLevel, c, d, e, practiceSettings, g, h)
#define CAMPAIGNLEVELSTART CampaignLevelStart(self, a, b, c, d, e, f, g)
#else
#error Define BSVERSION can be BS__1_17, BS__1_16 or BS__1_13_2
#endif

SONGSTARTHOOK {
    stManager->statusLock.lock();
    stManager->location = Solo_Song;
    ResetScores();
    stManager->isPractice = practiceSettings; // If practice settings isn't null, then we're in practice mode
    if (CoverStatus == Failed) GetCover(reinterpret_cast<GlobalNamespace::PreviewBeatmapLevelSO*>(previewBeatmapLevel)); // Try loading the Cover again if failed previously
    stManager->statusLock.unlock();
    SONGSTART;
}

CAMPAIGNLEVELSTARTHOOK {
    stManager->statusLock.lock();
    stManager->location = Campaign;
    ResetScores();
    stManager->statusLock.unlock();
    CAMPAIGNLEVELSTART;
}

ST_MAKE_HOOK(RelativeScoreAndImmediateRankCounter_UpdateRelativeScoreAndImmediateRank, &RelativeScoreAndImmediateRankCounter::UpdateRelativeScoreAndImmediateRank, void, RelativeScoreAndImmediateRankCounter* self, int score, int modifiedScore, int maxPossibleScore, int maxPossibleModifiedScore) {
    RelativeScoreAndImmediateRankCounter_UpdateRelativeScoreAndImmediateRank(self, score, modifiedScore, maxPossibleScore, maxPossibleModifiedScore);
    stManager->statusLock.lock();
    stManager->score = modifiedScore;
    stManager->rank = to_utf8(csstrtostr(RankModel::GetRankName(self->get_immediateRank())));
    stManager->accuracy = self->get_relativeScore();
    stManager->statusLock.unlock();
}

ST_MAKE_HOOK(GameEnergyUIPanel_HandleGameEnergyDidChange, &GameEnergyUIPanel::HandleGameEnergyDidChange, void, GameEnergyUIPanel* self, float energy) {
    GameEnergyUIPanel_HandleGameEnergyDidChange(self, energy);
    stManager->statusLock.lock();
    stManager->energy = energy;
    stManager->statusLock.unlock();
}

ST_MAKE_HOOK(ServerCodeView_RefreshText, &ServerCodeView::RefreshText, void, ServerCodeView* self, bool refreshText) {
    ServerCodeView_RefreshText(self, refreshText);
    if (self->serverCode) {
        stManager->statusLock.lock();
        stManager->mpGameId = to_utf8(csstrtostr(self->serverCode));
        stManager->mpGameIdShown = self->codeIsShown;
        stManager->statusLock.unlock();
    }
}

ST_MAKE_HOOK(ScoreController_Update, &ScoreController::Update, void, ScoreController* self) {
    stManager->statusLock.lock();
    stManager->combo = self->combo;
    stManager->statusLock.unlock();
    ScoreController_Update(self);
}

// Multiplayer song starting is handled differently
ST_MAKE_HOOK(MultiplayerSongStart, &MultiplayerLevelScenesTransitionSetupDataSO::Init, void, MultiplayerLevelScenesTransitionSetupDataSO* self, Il2CppString* gameMode, IPreviewBeatmapLevel* previewBeatmapLevel, BeatmapDifficulty beatmapDifficulty, BeatmapCharacteristicSO* beatmapCharacteristic, IDifficultyBeatmap* difficultyBeatmap, ColorScheme* overrideColorScheme, GameplayModifiers* gameplayModifiers, PlayerSpecificSettings* playerSpecificSettings, PracticeSettings* practiceSettings, bool useTestNoteCutSoundEffects) {
    stManager->statusLock.lock();
    stManager->location = MP_Song;
    ResetScores();
    if (CoverStatus == Failed) GetCover(reinterpret_cast<GlobalNamespace::PreviewBeatmapLevelSO*>(previewBeatmapLevel)); // Try loading the Cover again if failed previously
    stManager->statusLock.unlock();


    MultiplayerSongStart(self, gameMode, previewBeatmapLevel, beatmapDifficulty, beatmapCharacteristic, difficultyBeatmap, overrideColorScheme, gameplayModifiers, playerSpecificSettings, practiceSettings, useTestNoteCutSoundEffects);
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
    stManager->statusLock.lock();
    stManager->location = Menu;
    stManager->mpGameId = "";
    stManager->mpGameIdShown = false;
    stManager->statusLock.unlock();
}

#if defined(BS__1_13_2) || defined(BS__1_16) && BS__1_16 < 4
ST_MAKE_HOOK(MultiplayerJoinLobby, &GameServerLobbyFlowCoordinator::DidActivate, void, GameServerLobbyFlowCoordinator* self, bool firstActivation, bool addedToHierarchy, bool screenSystemEnabling)    {
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
    stManager->location = MP_Lobby;
    stManager->statusLock.unlock();

    MultiplayerJoinLobby(self, firstActivation, addedToHierarchy, screenSystemEnabling);
}
#else
ST_MAKE_HOOK(MultiplayerJoinLobby, &MultiplayerLobbyConnectionController::HandleMultiplayerSessionManagerConnected, void, MultiplayerLobbyConnectionController* self) {
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
    stManager->location = MP_Lobby;
    stManager->statusLock.unlock();

    MultiplayerJoinLobby(self);
}
#endif
ST_MAKE_HOOK(SongEnd, &StandardLevelGameplayManager::OnDestroy, void, StandardLevelGameplayManager* self) {
    stManager->statusLock.lock();
    stManager->paused = false; // If we are paused, unpause us, since we are returning to the menu
    stManager->location = Menu;
    stManager->statusLock.unlock();
    SongEnd(self);
}

ST_MAKE_HOOK(MultiplayerSongEnd, &MultiplayerLocalActivePlayerGameplayManager::OnDisable, void, MultiplayerLocalActivePlayerGameplayManager* self) {
    stManager->statusLock.lock();
    stManager->location = Menu;
    stManager->paused = false; // If we are paused, unpause us, since we are returning to the menu
    stManager->statusLock.unlock();
    MultiplayerSongEnd(self);
}

ST_MAKE_HOOK(TutorialStart, &TutorialSongController::Awake, void, TutorialSongController* self)   {
    stManager->statusLock.lock();
    stManager->location = Tutorial;
    ResetScores();
    stManager->statusLock.unlock();
    TutorialStart(self);
}
ST_MAKE_HOOK(TutorialEnd, &TutorialSongController::OnDestroy, void, TutorialSongController* self)   {
    stManager->statusLock.lock();
    stManager->location = Menu;
    stManager->paused = false; // If we are paused, unpause us, since we are returning to the menu
    stManager->statusLock.unlock();
    TutorialEnd(self);
}

ST_MAKE_HOOK(CampaignLevelEnd, &MissionLevelGameplayManager::OnDestroy, void, MissionLevelGameplayManager* self)   {
    stManager->statusLock.lock();
    stManager->location = Menu;
    stManager->paused = false; // If we are paused, unpause us, since we are returning to the menu
    stManager->statusLock.unlock();
    CampaignLevelEnd(self);
}

ST_MAKE_HOOK(GamePause, &PauseController::Pause, void, PauseController* self)   {
    stManager->statusLock.lock();
    stManager->paused = true;
    stManager->statusLock.unlock();
    GamePause(self);
}
ST_MAKE_HOOK(GameResume, &PauseController::HandlePauseMenuManagerDidPressContinueButton, void, PauseController* self)   {
    stManager->statusLock.lock();
    stManager->paused = false;
    stManager->statusLock.unlock();
    GameResume(self);
}

ST_MAKE_HOOK(AudioUpdate, &AudioTimeSyncController::Update, void, AudioTimeSyncController* self) {
    AudioUpdate(self);

    //float time = CRASH_UNLESS(il2cpp_utils::RunMethodUnsafe<float>(self, "get_songTime"));
    //float endTime = CRASH_UNLESS(il2cpp_utils::RunMethodUnsafe<float>(self, "get_songEndTime"));

    float time = self->get_songTime();
    float endTime = self->get_songEndTime();

    stManager->statusLock.lock();
    stManager->time = (int)time;
    stManager->endTime = (int)endTime;
    stManager->statusLock.unlock();
}

ST_MAKE_HOOK(ScoreController_HandleNoteWasMissed, &ScoreController::HandleNoteWasMissed, void, ScoreController* self, NoteController* note) {
    ScoreController_HandleNoteWasMissed(self, note);
    stManager->statusLock.lock();
    stManager->missedNotes++;
    stManager->statusLock.unlock();
}

#if defined(BS__1_13_2) || defined(BS__1_16) && BS__1_16 < 4
ST_MAKE_HOOK(ScoreController_HandleNoteWasCut, &ScoreController::HandleNoteWasCut, void, ScoreController* self, NoteController* noteController, NoteCutInfo& noteCutInfo)
#else
ST_MAKE_HOOK(ScoreController_HandleNoteWasCut, &ScoreController::HandleNoteWasCut, void, ScoreController* self, NoteController* noteController, ByRef<NoteCutInfo> noteCutInfo)
#endif
{
    ScoreController_HandleNoteWasCut(self, noteController, noteCutInfo);
    stManager->statusLock.lock();
#if defined(BS__1_13_2) || defined(BS__1_16) && BS__1_16 < 4
    if (noteCutInfo.get_allIsOK()) stManager->goodCuts++;
#else
    if (noteCutInfo.heldRef.get_allIsOK()) stManager->goodCuts++;
#endif
    else stManager->badCuts++;
    stManager->statusLock.unlock();
}

ST_MAKE_HOOK(FPSCounter_Update, &FPSCounter::Update, void, FPSCounter* self) {
    FPSCounter_Update(self);
    
    stManager->statusLock.lock();
    stManager->fps = self->currentFPS;
    stManager->statusLock.unlock();
}

ST_MAKE_HOOK(PlayerTransforms_Update, &PlayerTransforms::Update, void, PlayerTransforms* self) {
    PlayerTransforms_Update(self);
    if (!self->overrideHeadPos) {
        stManager->statusLock.lock();
        if (self->headTransform)
            stManager->Head = self->headTransform;
        if (self->rightHandTransform)
            stManager->VR_Right = self->rightHandTransform;
        if (self->leftHandTransform)
            stManager->VR_Left = self->leftHandTransform;
        stManager->statusLock.unlock();
    }
}

bool FPSObjectCreated = false;

std::string GetHeadsetType() {
    GlobalNamespace::OVRPlugin::SystemHeadset HeadsetType = GlobalNamespace::OVRPlugin::GetSystemHeadsetType();
    std::string result;
    switch (HeadsetType.value) {
    case HeadsetType.Oculus_Quest:
        return result = "Oculus Quest";
#ifdef BS__1_13_2
    case HeadsetType.Oculus_Go:
        return result = "Oculus Go";
    case 9:
        return result = "Oculus Quest 2";
#elif defined(BS__1_16) || defined(BS__1_17)
    case 7:
        return result = "Oculus Go";
    case HeadsetType.Oculus_Quest_2:
        return result = "Oculus Quest 2";
#endif
    case 10:
        return result = "Oculus Quest 3/2 Pro";
    default:
        return result = "Unknown " + to_utf8(csstrtostr(GlobalNamespace::OVRPlugin::get_productName()));
    }    
}

ST_MAKE_HOOK(SceneManager_ActiveSceneChanged, &UnityEngine::SceneManagement::SceneManager::Internal_ActiveSceneChanged, void, UnityEngine::SceneManagement::Scene previousActiveScene, UnityEngine::SceneManagement::Scene newActiveScene) {
    SceneManager_ActiveSceneChanged(previousActiveScene, newActiveScene);
    if (newActiveScene.IsValid()) {
        std::string sceneName = to_utf8(csstrtostr(newActiveScene.get_name()));
        std::string shaderWarmup = "ShaderWarmup";
        std::string EmptyTransition = "EmptyTransition";
        if (sceneName == EmptyTransition) stManager->headsetType = GetHeadsetType();
        else if (sceneName == shaderWarmup) {
#if (defined(BS__1_16) || defined(BS__1_17)) && !defined(BS__1_13_2)
            auto FPSCObject = UnityEngine::GameObject::New_ctor(il2cpp_utils::newcsstr("FPSC"));
#elif defined(BS__1_13_2)
            auto FPSCObject = UnityEngine::GameObject::New_ctor(il2cpp_utils::createcsstr("FPSC"));
#endif
            UnityEngine::Object::DontDestroyOnLoad(FPSCObject->AddComponent<FPSCounter*>());
        }
        FPSObjectCreated = true;
    }
}

ST_MAKE_HOOK(OptionsViewController_DidActivate, &OptionsViewController::DidActivate, void, OptionsViewController* self, bool firstActivation, bool addedToHierarchy, bool screenSystemEnabling) {
    OptionsViewController_DidActivate(self, firstActivation, addedToHierarchy, screenSystemEnabling);
    stManager->statusLock.lock();
    stManager->location = Options;
    stManager->statusLock.unlock();
}

ST_MAKE_HOOK(MainMenuViewController_DidActivate, &MainMenuViewController::DidActivate, void, MainMenuViewController* self, bool firstActivation, bool addedToHierarchy, bool screenSystemEnabling) {
    MainMenuViewController_DidActivate(self, firstActivation, addedToHierarchy, screenSystemEnabling);
    stManager->statusLock.lock();
    stManager->location = Menu;
    stManager->statusLock.unlock();
}

extern "C" void setup(ModInfo& info) {
    info.id = ID;
    info.version = VERSION;
    STModInfo = info;

    //#ifdef DEBUG_BUILD
    //// Disable loggers
    //// Here's where you can disable individual context Loggers
    //#ifdef HTTP_LOGGING
    //// @brief Disables HTTP Server Logger
    //getLogger().DisableContext("ServerHTTP");
    //#endif
    //#ifdef SOCKET_LOGGING
    //// @brief Disables Socket Server Logger
    //getLogger().DisableContext("ServerSocket");
    //#endif
    //#ifdef MULTICAST_LOGGING
    //// @brief Disables Multicast Server Logger
    //getLogger().DisableContext("ServerMulticast");
    //#endif
    //#elif
    //// @brief Disables all Server Loggers
    //getLogger().DisableContext("ServerMulticast");
    //getLogger().DisableContext("ServerSocket");
    //getLogger().DisableContext("ServerHTTP");
    ////getLogger().DisableContext("Server");
    //#endif

    getLogger().info("Modloader name: %s", Modloader::getInfo().name.c_str());

    getLogger().info("Completed setup!");
}

extern "C" void load() {
    getLogger().debug("Installing hooks...");
    il2cpp_functions::Init();
    QuestUI::Init();

    getModConfig().Init(STModInfo);

    #ifndef REGISTER_FUNCTION
    custom_types::Register::AutoRegister();
    #else
    custom_types::Register::RegisterType<StreamerTools::stSettingViewController>();
    #endif
    QuestUI::Register::RegisterModSettingsViewController<StreamerTools::stSettingViewController*>(STModInfo);

    // Install our function hooks
    Logger& logger = getLogger();
    ST_INSTALL_HOOK(logger, PlayerTransforms_Update, il2cpp_utils::FindMethodUnsafe("", "PlayerTransforms", "Update", 0));
    ST_INSTALL_HOOK(logger, RefreshContent, il2cpp_utils::FindMethodUnsafe("", "StandardLevelDetailView", "RefreshContent", 0));
#if defined(BS__1_16)
    ST_INSTALL_HOOK(logger, SongStart, il2cpp_utils::FindMethodUnsafe("", "StandardLevelScenesTransitionSetupDataSO", "Init", 10));
    ST_INSTALL_HOOK(logger, CampaignLevelStart, il2cpp_utils::FindMethodUnsafe("", "MissionLevelScenesTransitionSetupDataSO", "Init", 8));
#elif defined(BS__1_13_2)
    INSTALL_HOOK_OFFSETLESS(logger, SongStart, il2cpp_utils::FindMethodUnsafe("", "StandardLevelScenesTransitionSetupDataSO", "Init", 9));
    INSTALL_HOOK_OFFSETLESS(logger, CampaignLevelStart, il2cpp_utils::FindMethodUnsafe("", "MissionLevelScenesTransitionSetupDataSO", "Init", 7));
#endif
    ST_INSTALL_HOOK(logger, RelativeScoreAndImmediateRankCounter_UpdateRelativeScoreAndImmediateRank, il2cpp_utils::FindMethodUnsafe("", "RelativeScoreAndImmediateRankCounter", "UpdateRelativeScoreAndImmediateRank", 4));
    ST_INSTALL_HOOK(logger, ScoreController_Update, il2cpp_utils::FindMethodUnsafe("", "ScoreController", "Update", 0));
    ST_INSTALL_HOOK(logger, SongEnd, il2cpp_utils::FindMethodUnsafe("", "StandardLevelGameplayManager", "OnDestroy", 0));
    ST_INSTALL_HOOK(logger, CampaignLevelEnd, il2cpp_utils::FindMethodUnsafe("", "MissionLevelGameplayManager", "OnDestroy", 0));
    ST_INSTALL_HOOK(logger, TutorialStart, il2cpp_utils::FindMethodUnsafe("", "TutorialSongController", "Awake", 0));
    ST_INSTALL_HOOK(logger, TutorialEnd, il2cpp_utils::FindMethodUnsafe("", "TutorialSongController", "OnDestroy", 0));
    ST_INSTALL_HOOK(logger, GamePause, il2cpp_utils::FindMethodUnsafe("", "PauseController", "Pause", 0));
    ST_INSTALL_HOOK(logger, GameResume, il2cpp_utils::FindMethodUnsafe("", "PauseController", "HandlePauseMenuManagerDidPressContinueButton", 0));
    ST_INSTALL_HOOK(logger, AudioUpdate, il2cpp_utils::FindMethodUnsafe("", "AudioTimeSyncController", "Update", 0));
    ST_INSTALL_HOOK(logger, MultiplayerSongStart, il2cpp_utils::FindMethodUnsafe("", "MultiplayerLevelScenesTransitionSetupDataSO", "Init", 10));
#if defined(BS__1_16) && BS__1_16 < 4
    ST_INSTALL_HOOK(logger, MultiplayerJoinLobby, il2cpp_utils::FindMethodUnsafe("", "GameServerLobbyFlowCoordinator", "DidActivate", 3));
#else
    ST_INSTALL_HOOK(logger, MultiplayerJoinLobby, il2cpp_utils::FindMethodUnsafe("", "MultiplayerLobbyConnectionController", "HandleMultiplayerSessionManagerConnected", 0));
#endif
    ST_INSTALL_HOOK(logger, MultiplayerSongEnd, il2cpp_utils::FindMethodUnsafe("", "MultiplayerLocalActivePlayerGameplayManager", "OnDisable", 0));
    ST_INSTALL_HOOK(logger, GameEnergyUIPanel_HandleGameEnergyDidChange, il2cpp_utils::FindMethodUnsafe("", "GameEnergyUIPanel", "HandleGameEnergyDidChange", 1));
    ST_INSTALL_HOOK(logger, ServerCodeView_RefreshText, il2cpp_utils::FindMethodUnsafe("", "ServerCodeView", "RefreshText", 1));
    ST_INSTALL_HOOK(logger, ScoreController_HandleNoteWasMissed, il2cpp_utils::FindMethodUnsafe("", "ScoreController", "HandleNoteWasMissed", 1));
    ST_INSTALL_HOOK(logger, ScoreController_HandleNoteWasCut, il2cpp_utils::FindMethodUnsafe("", "ScoreController", "HandleNoteWasCut", 2));
    ST_INSTALL_HOOK(logger, FPSCounter_Update, il2cpp_utils::FindMethodUnsafe("", "FPSCounter", "Update", 0));
    ST_INSTALL_HOOK(logger, SceneManager_ActiveSceneChanged, il2cpp_utils::FindMethodUnsafe("UnityEngine.SceneManagement", "SceneManager", "Internal_ActiveSceneChanged", 2));
    ST_INSTALL_HOOK(logger, OptionsViewController_DidActivate, il2cpp_utils::FindMethodUnsafe("", "OptionsViewController", "DidActivate", 3));
    ST_INSTALL_HOOK(logger, MainMenuViewController_DidActivate, il2cpp_utils::FindMethodUnsafe("", "MainMenuViewController", "DidActivate", 3));


    getLogger().debug("Installed all hooks!");

    stManager = new STManager(getLogger());
    stManager->gameVersion = to_utf8(csstrtostr(UnityEngine::Application::get_version()));

}