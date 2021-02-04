#pragma once

#include "beatsaber-hook/shared/utils/logging.hpp"
#include "beatsaber-hook/shared/config/config-utils.hpp"
#include <thread>
#include <optional>
#include <mutex>

class LevelInfo  {
public:
    std::string name;
    std::string levelAuthor;
    std::string songAuthor;
    std::string selectedDifficulty;
};

class MultiplayerLobbyInfo {
public:
    int numberOfPlayers;
    int maxPlayers;
};

enum TimeMode {
    REMAINING,
    LEFT
};

class PresenceManager    {
private:
    Logger& logger;
    const ConfigDocument& config;
    std::thread networkThread;

    void threadEntrypoint();
    bool runServer();
    bool createModuleJSON();
    std::string constructResponse();
    std::string handlePlaceholders(std::string str);
    std::string replaceAll(std::string& str, std::string key, std::string replacement);
public:
    std::mutex statusLock; // Lock to make sure that stuff doesn't get overwritten while being read by the network thread

    std::optional<LevelInfo> playingLevel = std::nullopt;
    bool playingCampaign = false;
    bool playingTutorial = false;
    bool isPractice = false;
    bool paused = false;
    std::optional<MultiplayerLobbyInfo> multiplayerLobby = std::nullopt;
    std::string previousSectionUsed = "menuPresence";
    
    int timeLeft = 0; // The time when first going into the menu, or going into a multiplayer game. Or the time left if it's a song.

    PresenceManager(Logger& logger, const ConfigDocument& config);
};