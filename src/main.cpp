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

#include "System/Action.hpp"
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

#include "GlobalNamespace/MultiplayerLobbyConnectionController.hpp"

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
#include "GlobalNamespace/MultiplayerSpectatorController.hpp"
#include "GlobalNamespace/MultiplayerSessionManager_SessionType.hpp"
using namespace GlobalNamespace;

#if !defined(MAKE_HOOK_MATCH)
#error No Compatible HOOK macro found
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

//bool GotBeatmapInfo = false;

MAKE_HOOK_MATCH(RefreshContent, &StandardLevelDetailView::RefreshContent, void, StandardLevelDetailView* self) {
    RefreshContent(self);

    // Null Check Level before trying to get any data
    if (self->dyn__level()) {
        stManager->statusLock.lock();
        stManager->levelName = std::string(reinterpret_cast<IPreviewBeatmapLevel*>(self->dyn__level())->get_songName());
        stManager->levelSubName = std::string(reinterpret_cast<IPreviewBeatmapLevel*>(self->dyn__level())->get_songSubName());
        stManager->levelAuthor = std::string(reinterpret_cast<IPreviewBeatmapLevel*>(self->dyn__level())->get_levelAuthorName());
        stManager->songAuthor = std::string(reinterpret_cast<IPreviewBeatmapLevel*>(self->dyn__level())->get_songAuthorName());
        stManager->id = std::string(reinterpret_cast<IPreviewBeatmapLevel*>(self->dyn__level())->get_levelID());
        stManager->bpm = reinterpret_cast<IPreviewBeatmapLevel*>(self->dyn__level())->get_beatsPerMinute();
        // Check if level can be assigned as CustomPreviewBeatmapLevel
        bool CustomLevel = (il2cpp_functions::class_is_assignable_from(classof(CustomPreviewBeatmapLevel*), il2cpp_functions::object_get_class(reinterpret_cast<Il2CppObject*>(self->dyn__level()))));
        stManager->njs = self->dyn__selectedDifficultyBeatmap()->get_noteJumpMovementSpeed();
        stManager->difficulty = self->dyn__selectedDifficultyBeatmap()->get_difficulty().value;
        stManager->coverFetchable = false;
        GetCover(reinterpret_cast<GlobalNamespace::PreviewBeatmapLevelSO*>(self->dyn__level()));
        stManager->statusLock.unlock();
        //GotBeatmapInfo = true;
    }
    else {
        //GotBeatmapInfo = false;
        getLogger().info("BeatmapLevelSO is nullptr");
    }
}

MAKE_HOOK_MATCH(SongStart, &StandardLevelScenesTransitionSetupDataSO::Init, void, StandardLevelScenesTransitionSetupDataSO* self, StringW gameMode, IDifficultyBeatmap* difficultyBeatmap, IPreviewBeatmapLevel* previewBeatmapLevel, OverrideEnvironmentSettings* overrideEnvironmentSettings, ColorScheme* overrideColorScheme, GameplayModifiers* gameplayModifiers, PlayerSpecificSettings* playerSpecificSettings, PracticeSettings* practiceSettings, StringW backButtonText, bool useTestNoteCutSoundEffects) {
    stManager->statusLock.lock();
    stManager->location = Solo_Song;
    ResetScores();
    stManager->isPractice = practiceSettings; // If practice settings isn't null, then we're in practice mode
    if (CoverStatus == Failed) GetCover(reinterpret_cast<GlobalNamespace::PreviewBeatmapLevelSO*>(previewBeatmapLevel)); // Try loading the Cover again if failed previously
    stManager->statusLock.unlock();
    SongStart(self, gameMode, difficultyBeatmap, previewBeatmapLevel, overrideEnvironmentSettings, overrideColorScheme, gameplayModifiers, playerSpecificSettings, practiceSettings, backButtonText, useTestNoteCutSoundEffects);
}

MAKE_HOOK_MATCH(CampaignLevelStart, &MissionLevelScenesTransitionSetupDataSO::Init, void, MissionLevelScenesTransitionSetupDataSO* self, StringW missionId, IDifficultyBeatmap* difficultyBeatmap, IPreviewBeatmapLevel* previewBeatmapLevel, ArrayW<MissionObjective*> missionObjectives, ColorScheme* overrideColorScheme, GameplayModifiers* gameplayModifiers, PlayerSpecificSettings* playerSpecificSettings, StringW backButtonText) {
    stManager->statusLock.lock();
    stManager->location = Campaign;
    ResetScores();
    stManager->statusLock.unlock();
    CampaignLevelStart(self, missionId, difficultyBeatmap, previewBeatmapLevel, missionObjectives, overrideColorScheme, gameplayModifiers, playerSpecificSettings, backButtonText);
}

MAKE_HOOK_MATCH(RelativeScoreAndImmediateRankCounter_UpdateRelativeScoreAndImmediateRank, &RelativeScoreAndImmediateRankCounter::UpdateRelativeScoreAndImmediateRank, void, RelativeScoreAndImmediateRankCounter* self, int score, int modifiedScore, int maxPossibleScore, int maxPossibleModifiedScore) {
    RelativeScoreAndImmediateRankCounter_UpdateRelativeScoreAndImmediateRank(self, score, modifiedScore, maxPossibleScore, maxPossibleModifiedScore);
    stManager->statusLock.lock();
    stManager->score = modifiedScore;
    stManager->rank = std::string(RankModel::GetRankName(self->get_immediateRank()));
    stManager->accuracy = self->get_relativeScore();
    stManager->statusLock.unlock();
}

MAKE_HOOK_MATCH(GameEnergyUIPanel_HandleGameEnergyDidChange, &GameEnergyUIPanel::HandleGameEnergyDidChange, void, GameEnergyUIPanel* self, float energy) {
    GameEnergyUIPanel_HandleGameEnergyDidChange(self, energy);
    stManager->statusLock.lock();
    stManager->energy = energy;
    stManager->statusLock.unlock();
}

MAKE_HOOK_MATCH(ServerCodeView_RefreshText, &ServerCodeView::RefreshText, void, ServerCodeView* self, bool refreshText) {
    ServerCodeView_RefreshText(self, refreshText);
    if (self->dyn__serverCode()) {
        stManager->statusLock.lock();
        stManager->mpGameId = to_utf8(csstrtostr(self->dyn__serverCode()));
        stManager->mpGameIdShown = self->dyn__codeIsShown();
        stManager->statusLock.unlock();
    }
}

MAKE_HOOK_MATCH(ScoreController_Update, &ScoreController::Update, void, ScoreController* self) {
    stManager->statusLock.lock();
    stManager->combo = self->dyn__combo();
    stManager->statusLock.unlock();
    ScoreController_Update(self);
}

// Multiplayer song starting is handled differently
MAKE_HOOK_MATCH(MultiplayerSongStart, &MultiplayerLevelScenesTransitionSetupDataSO::Init, void, MultiplayerLevelScenesTransitionSetupDataSO* self, StringW gameMode, IPreviewBeatmapLevel* previewBeatmapLevel, BeatmapDifficulty beatmapDifficulty, BeatmapCharacteristicSO* beatmapCharacteristic, IDifficultyBeatmap* difficultyBeatmap, ColorScheme* overrideColorScheme, GameplayModifiers* gameplayModifiers, PlayerSpecificSettings* playerSpecificSettings, PracticeSettings* practiceSettings, bool useTestNoteCutSoundEffects) {
    stManager->statusLock.lock();
    stManager->location = MP_Song;
    ResetScores();
    if (previewBeatmapLevel) {
        stManager->levelName = std::string(previewBeatmapLevel->get_songName());
        stManager->levelSubName = std::string(previewBeatmapLevel->get_songSubName());
        stManager->levelAuthor = std::string(previewBeatmapLevel->get_levelAuthorName());
        stManager->songAuthor = std::string(previewBeatmapLevel->get_songAuthorName());
        stManager->id = std::string(previewBeatmapLevel->get_levelID());
        stManager->bpm = previewBeatmapLevel->get_beatsPerMinute();
        stManager->njs = difficultyBeatmap->get_noteJumpMovementSpeed();
        stManager->difficulty = beatmapDifficulty.value;
        stManager->coverFetchable = false;
        GetCover(reinterpret_cast<GlobalNamespace::PreviewBeatmapLevelSO*>(previewBeatmapLevel));
    }
    else {
        getLogger().debug("IPreviewBeatmapLevel is %p", previewBeatmapLevel);
    }
    stManager->statusLock.unlock();


    MultiplayerSongStart(self, gameMode, previewBeatmapLevel, beatmapDifficulty, beatmapCharacteristic, difficultyBeatmap, overrideColorScheme, gameplayModifiers, playerSpecificSettings, practiceSettings, useTestNoteCutSoundEffects);
}

void onLobbyJoin(MultiplayerSessionManager* sessionManager) {
    stManager->statusLock.lock();
    stManager->location = MP_Lobby;
    int players = sessionManager->get_connectedPlayerCount();
    if (players > 0)
        stManager->players = players;
    else stManager->players = players + 1;
    stManager->maxPlayers = sessionManager->get_maxPlayerCount();
    stManager->statusLock.unlock();
}

void onPlayerJoin(MultiplayerSessionManager* sessionManager, IConnectedPlayer * player) {
    stManager->statusLock.lock();
    if (!(player->get_isMe() && player->get_isConnectionOwner()))
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

MAKE_HOOK_MATCH(MultiplayerJoinLobby, &MultiplayerSessionManager::StartSession, void, MultiplayerSessionManager* self, GlobalNamespace::MultiplayerSessionManager_SessionType sessionType, ConnectedPlayerManager* connectedPlayerManager) {
    MultiplayerJoinLobby(self, sessionType, connectedPlayerManager);

    int maxPlayers = self->get_maxPlayerCount();
    int numActivePlayers = self->get_connectedPlayerCount();

    // Register player join and leave events
    self->add_playerDisconnectedEvent(
        il2cpp_utils::MakeDelegate<System::Action_1<IConnectedPlayer*>*>(classof(System::Action_1<IConnectedPlayer*>*), static_cast<Il2CppObject*>(nullptr), onPlayerLeave)
    );

    self->add_playerConnectedEvent(
        il2cpp_utils::MakeDelegate<System::Action_1<IConnectedPlayer*>*>(classof(System::Action_1<IConnectedPlayer*>*), self, onPlayerJoin)
    );

    // Register connect and disconnect from lobby events
    self->add_disconnectedEvent(
        il2cpp_utils::MakeDelegate<System::Action_1<GlobalNamespace::DisconnectedReason>*>(classof(System::Action_1<GlobalNamespace::DisconnectedReason>*), static_cast<Il2CppObject*>(nullptr), onLobbyDisconnect)
    );

    self->add_connectedEvent(il2cpp_utils::MakeDelegate<System::Action*>(classof(System::Action*), self, onLobbyJoin));
}
MAKE_HOOK_MATCH(SongEnd, &StandardLevelGameplayManager::OnDestroy, void, StandardLevelGameplayManager* self) {
    stManager->statusLock.lock();
    stManager->paused = false; // If we are paused, unpause us, since we are returning to the menu
    stManager->location = Menu;
    stManager->statusLock.unlock();
    SongEnd(self);
}

MAKE_HOOK_MATCH(MultiplayerSongFinish, &MultiplayerLocalActivePlayerGameplayManager::HandleSongDidFinish, void, MultiplayerLocalActivePlayerGameplayManager* self) {
    stManager->statusLock.lock();
    stManager->location = MP_Lobby;
    stManager->paused = false; // If we are paused, unpause us, since we are returning to the menu
    stManager->statusLock.unlock();
    MultiplayerSongFinish(self);
}

MAKE_HOOK_MATCH(MultiplayerSpectateStart, &MultiplayerSpectatorController::Start, void, MultiplayerSpectatorController* self) {
    stManager->statusLock.lock();
    stManager->location = Spectator;
    stManager->paused = false; // If we are paused, unpause us, since we are returning to the menu
    stManager->statusLock.unlock();
    MultiplayerSpectateStart(self);
}

MAKE_HOOK_MATCH(MultiplayerSpectateDestroy, &MultiplayerSpectatorController::OnDestroy, void, MultiplayerSpectatorController* self) {
    stManager->statusLock.lock();
    stManager->location = MP_Lobby;
    stManager->statusLock.unlock();
    MultiplayerSpectateDestroy(self);
}

MAKE_HOOK_MATCH(TutorialStart, &TutorialSongController::Awake, void, TutorialSongController* self)   {
    stManager->statusLock.lock();
    stManager->location = Tutorial;
    ResetScores();
    stManager->statusLock.unlock();
    TutorialStart(self);
}
MAKE_HOOK_MATCH(TutorialEnd, &TutorialSongController::OnDestroy, void, TutorialSongController* self)   {
    stManager->statusLock.lock();
    stManager->location = Menu;
    stManager->paused = false; // If we are paused, unpause us, since we are returning to the menu
    stManager->statusLock.unlock();
    TutorialEnd(self);
}

MAKE_HOOK_MATCH(CampaignLevelEnd, &MissionLevelGameplayManager::OnDestroy, void, MissionLevelGameplayManager* self)   {
    stManager->statusLock.lock();
    stManager->location = Menu;
    stManager->paused = false; // If we are paused, unpause us, since we are returning to the menu
    stManager->statusLock.unlock();
    CampaignLevelEnd(self);
}

MAKE_HOOK_MATCH(GamePause, &PauseController::Pause, void, PauseController* self)   {
    stManager->statusLock.lock();
    stManager->paused = true;
    stManager->statusLock.unlock();
    GamePause(self);
}
MAKE_HOOK_MATCH(GameResume, &PauseController::HandlePauseMenuManagerDidPressContinueButton, void, PauseController* self)   {
    stManager->statusLock.lock();
    stManager->paused = false;
    stManager->statusLock.unlock();
    GameResume(self);
}

MAKE_HOOK_MATCH(AudioUpdate, &AudioTimeSyncController::Update, void, AudioTimeSyncController* self) {
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

MAKE_HOOK_MATCH(ScoreController_HandleNoteWasMissed, &ScoreController::HandleNoteWasMissed, void, ScoreController* self, NoteController* note) {
    ScoreController_HandleNoteWasMissed(self, note);
    stManager->statusLock.lock();
    stManager->missedNotes++;
    stManager->statusLock.unlock();
}

MAKE_HOOK_MATCH(ScoreController_HandleNoteWasCut, &ScoreController::HandleNoteWasCut, void, ScoreController* self, NoteController* noteController, ByRef<NoteCutInfo> noteCutInfo)
{
    ScoreController_HandleNoteWasCut(self, noteController, noteCutInfo);
    stManager->statusLock.lock();
    if (noteCutInfo.heldRef.get_allIsOK()) stManager->goodCuts++;
    else stManager->badCuts++;
    stManager->statusLock.unlock();
}

MAKE_HOOK_MATCH(FPSCounter_Update, &FPSCounter::Update, void, FPSCounter* self) {
    FPSCounter_Update(self);
    
    stManager->statusLock.lock();
    stManager->fps = self->get_currentFPS();
    stManager->statusLock.unlock();
}

MAKE_HOOK_MATCH(PlayerTransforms_Update, &PlayerTransforms::Update, void, PlayerTransforms* self) {
    PlayerTransforms_Update(self);
    if (!self->dyn__overrideHeadPos()) {
        stManager->statusLock.lock();
        if (self->dyn__headTransform())
            stManager->Head = self->dyn__headTransform();
        if (self->dyn__rightHandTransform())
            stManager->VR_Right = self->dyn__rightHandTransform();
        if (self->dyn__leftHandTransform())
            stManager->VR_Left = self->dyn__leftHandTransform();
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
    case 7:
        return result = "Oculus Go";
    case HeadsetType.Oculus_Quest_2:
        return result = "Oculus Quest 2";
    case 10:
        return result = "Oculus Quest 3/2 Pro";
    default:
        return result = "Unknown " + std::string(GlobalNamespace::OVRPlugin::get_productName());
    }    
}

MAKE_HOOK_MATCH(SceneManager_ActiveSceneChanged, &UnityEngine::SceneManagement::SceneManager::Internal_ActiveSceneChanged, void, UnityEngine::SceneManagement::Scene previousActiveScene, UnityEngine::SceneManagement::Scene newActiveScene) {
    SceneManager_ActiveSceneChanged(previousActiveScene, newActiveScene);
    if (newActiveScene.IsValid()) {
        std::string sceneName = to_utf8(csstrtostr(newActiveScene.get_name()));
        std::string shaderWarmup = "ShaderWarmup";
        std::string EmptyTransition = "EmptyTransition";
        if (sceneName == EmptyTransition) stManager->headsetType = GetHeadsetType();
        else if (sceneName == shaderWarmup) {
            auto FPSCObject = UnityEngine::GameObject::New_ctor(il2cpp_utils::newcsstr("FPSC"));
            UnityEngine::Object::DontDestroyOnLoad(FPSCObject->AddComponent<FPSCounter*>());
        }
        FPSObjectCreated = true;
    }
}

MAKE_HOOK_MATCH(OptionsViewController_DidActivate, &OptionsViewController::DidActivate, void, OptionsViewController* self, bool firstActivation, bool addedToHierarchy, bool screenSystemEnabling) {
    OptionsViewController_DidActivate(self, firstActivation, addedToHierarchy, screenSystemEnabling);
    stManager->statusLock.lock();
    stManager->location = Options;
    stManager->statusLock.unlock();
}

MAKE_HOOK_MATCH(MainMenuViewController_DidActivate, &MainMenuViewController::DidActivate, void, MainMenuViewController* self, bool firstActivation, bool addedToHierarchy, bool screenSystemEnabling) {
    MainMenuViewController_DidActivate(self, firstActivation, addedToHierarchy, screenSystemEnabling);
    stManager->statusLock.lock();
    stManager->location = Menu;
    stManager->statusLock.unlock();
}

extern "C" void setup(ModInfo& info) {
    info.id = ID;
    info.version = VERSION;
    STModInfo = info;

    getLogger().info("Modloader name: %s tag: %s", Modloader::getInfo().name.c_str(), Modloader::getInfo().tag.c_str());

    getLogger().info("Completed setup!");
}

extern "C" void load() {
    getLogger().debug("Installing hooks...");
    il2cpp_functions::Init();
    QuestUI::Init();

    getModConfig().Init(STModInfo);

    custom_types::Register::AutoRegister();
    QuestUI::Register::RegisterModSettingsViewController<StreamerTools::stSettingViewController*>(STModInfo);

    // Install our function hooks
    LoggerContextObject logger = getLogger().WithContext("Hook");
    INSTALL_HOOK(logger, PlayerTransforms_Update);
    INSTALL_HOOK(logger, RefreshContent);
    INSTALL_HOOK(logger, SongStart);
    INSTALL_HOOK(logger, CampaignLevelStart);
    INSTALL_HOOK(logger, RelativeScoreAndImmediateRankCounter_UpdateRelativeScoreAndImmediateRank);
    INSTALL_HOOK(logger, ScoreController_Update);
    INSTALL_HOOK(logger, SongEnd);
    INSTALL_HOOK(logger, CampaignLevelEnd);
    INSTALL_HOOK(logger, TutorialStart);
    INSTALL_HOOK(logger, TutorialEnd);
    INSTALL_HOOK(logger, GamePause);
    INSTALL_HOOK(logger, GameResume);
    INSTALL_HOOK(logger, AudioUpdate);
    INSTALL_HOOK(logger, MultiplayerSongStart);
    INSTALL_HOOK(logger, MultiplayerJoinLobby);
    INSTALL_HOOK(logger, MultiplayerSongFinish);
    INSTALL_HOOK(logger, MultiplayerSpectateStart);
    INSTALL_HOOK(logger, MultiplayerSpectateDestroy);
    INSTALL_HOOK(logger, GameEnergyUIPanel_HandleGameEnergyDidChange);
    INSTALL_HOOK(logger, ServerCodeView_RefreshText);
    INSTALL_HOOK(logger, ScoreController_HandleNoteWasMissed);
    INSTALL_HOOK(logger, ScoreController_HandleNoteWasCut);
    INSTALL_HOOK(logger, FPSCounter_Update);
    INSTALL_HOOK(logger, SceneManager_ActiveSceneChanged);
    INSTALL_HOOK(logger, OptionsViewController_DidActivate);
    INSTALL_HOOK(logger, MainMenuViewController_DidActivate);


    getLogger().debug("Installed all hooks!");

    stManager = new STManager(getLogger());
    stManager->gameVersion = to_utf8(csstrtostr(UnityEngine::Application::get_version()));

}