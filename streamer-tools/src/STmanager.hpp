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
    std::thread networkThreadMulticast;

    void threadEntrypoint();
    bool runServer();
    bool runServerHTTP();
    bool MulticastServer();

    void HandleRequestHTTP(int client_sock);
    void sendRequest(int client_sock);
    void ReadXBytes(int socket, unsigned int x, char* buffer);

    std::string constructResponse();
    std::string multicastResponse(std::string socket, std::string http);

    bool Connected = false;
public:
    std::mutex statusLock; // Lock to make sure that stuff doesn't get overwritten while being read by the network thread

    int location = 0; //0 = Menu, 1 = Solo song, 2 = mp song, 3 = tutorial, 4 = campaign, 5 = mp lobby
    bool isPractice = false;
    bool paused = false;
    
    int time = 0;
    int endTime = 0;

    int score = 0;
    std::string rank = "";
    int goodCuts = 0;
    int badCuts = 0;
    int missedNotes = 0;

    int fps = 0;

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

    std::string localIp = "127.0.0.1";

    STManager(Logger& logger, const ConfigDocument& config);
};

extern ModInfo STModInfo;