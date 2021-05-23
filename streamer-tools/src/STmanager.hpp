#pragma once

#include "beatsaber-hook/shared/utils/logging.hpp"
#include "beatsaber-hook/shared/config/config-utils.hpp"
#include <thread>
#include <optional>
#include <mutex>

class STManager    {
private:
    Logger& logger;
    const ConfigDocument& config;
    std::thread networkThread;
    std::thread networkThreadHTTP;

    void threadEntrypoint();
    bool runServer();
    bool runServerHTTP();
    bool createModuleJSON();
    std::string constructResponse();
public:
    std::mutex statusLock; // Lock to make sure that stuff doesn't get overwritten while being read by the network thread

    int type = 0; //0 = Menu, 1 = Solo, 2 = mp song, 3 = tutorial, 4 = campaign, 5 = mp lobby
    bool isPractice = false;
    bool paused = false;
    
    int time = 0;
    int endTime = 0;

    int score = 0;
    std::string rank = "";

    int combo = 0;
    float energy = 1.0f;
    float accuracy = 0.0f;

    std::string levelName = "";
    std::string levelSubName = "";
    std::string levelAuthor = "";
    std::string songAuthor = "";
    std::string id = "";
    int difficulty = 0;
    float bpm = 0.0f;
    float njs = 0.0f;

    int players = 0;
    int maxPlayers = 0;
    std::string mpGameId = "";
    bool mpGameIdShown = false;

    STManager(Logger& logger, const ConfigDocument& config);
};
