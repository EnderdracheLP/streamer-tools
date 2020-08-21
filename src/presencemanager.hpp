#pragma once

#include "../extern/beatsaber-hook/shared/utils/logging.hpp"
#include "../extern/beatsaber-hook/shared/config/config-utils.hpp"
#include <thread>
#include <optional>
#include <mutex>

class LevelInfo  {
public:
    std::string name;
    std::string levelAuthor;
    std::string songAuthor;
    std::string selectedDifficulty;
    float timeLeft = -1.0; // Set by the GameSongController update
};

class PresenceManager    {
private:
    const Logger& logger;
    const ConfigDocument& config;
    std::thread networkThread;

    void threadEntrypoint();
    bool runServer();
    bool createModuleJSON();
    std::string constructResponse();
    std::string handlePlaceholders(std::string str);
    std::string replaceAll(std::string& str, std::string key, std::string replacement);
    std::string escape_json(const std::string &s);
public:
    std::mutex statusLock; // Lock to make sure that stuff doesn't get overwritten while being read by the network thread

    std::optional<LevelInfo> playingLevel = std::nullopt;
    bool playingCampaign = false;
    bool playingTutorial = false;
    bool paused = false;
    int timeLeft = 0;

    PresenceManager(const Logger& logger, const ConfigDocument& config);
};