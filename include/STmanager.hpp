#pragma once

#include "beatsaber-hook/shared/utils/logging.hpp"

#include "questui/shared/QuestUI.hpp"
#include "config-utils/shared/config-utils.hpp"

#include "UnityEngine/GameObject.hpp"
#include "UnityEngine/Transform.hpp"

#include "UnityEngine/Texture2D.hpp"
/*  
    Use macros that for disablig enabling debug loggers, 
    set to 1 to enable or 0 to disable, 
    will all be disabled if DEBUG_BUILD is not defined
*/
#define HTTP_LOGGING        1
#define SOCKET_LOGGING      1
#define MULTICAST_LOGGING   1

#ifdef DEBUG_BUILD
#if HTTP_LOGGING == 1
#define LOG_DEBUG_HTTP(...) getLogger().WithContext("Server").WithContext("HTTP").debug(__VA_ARGS__) 
#else
#define LOG_DEBUG_HTTP(...)
#endif
#if SOCKET_LOGGING == 1
#define LOG_DEBUG_SOCKET(...) getLogger().WithContext("Server").WithContext("Socket").debug(__VA_ARGS__) 
#else
#define LOG_DEBUG_SOCKET(...) 
#endif
#if MULTICAST_LOGGING == 1
#define LOG_DEBUG_MULTICAST(...) getLogger().WithContext("Server").WithContext("Multicast").debug(__VA_ARGS__) 
#else
#define LOG_DEBUG_MULTICAST(...) 
#endif
#else
#define LOG_DEBUG_HTTP(...) 
#define LOG_DEBUG_SOCKET(...) 
#define LOG_DEBUG_MULTICAST(...) 
#endif

extern bool configFetched;
extern bool CoverChanged[4];

extern UnityEngine::GameObject* Decimals;
extern UnityEngine::GameObject* DEnergy;
extern UnityEngine::GameObject* D_MP_Code;
extern UnityEngine::GameObject* A_MP_Code;
extern UnityEngine::GameObject* A_Update;

extern void MakeConfigUI(bool Update);

extern ModInfo STModInfo;
Logger& getLogger();

class STManager    {
private:
    Logger& logger;

    std::thread networkThread;
    std::thread networkThreadHTTP;
    std::thread networkThreadMulticast;

    bool runServer();
    bool runServerHTTP();
    bool MulticastServer();

    void HandleRequestHTTP(int client_sock);
    void sendResponse(int client_sock);
    void ReadRequest(int socket, unsigned int x, char* buffer);
    void SendResponseHTTP(int client_sock, std::string response);
    bool HandleConfigChange(int client_sock, std::string BufferStr);

    std::string constructResponse();
    std::string constructConfigResponse();
    std::string multicastResponse(std::string socket, std::string http, std::string httpv6, std::string socketv6);
    bool MultipartResponseGen(int client_sock, std::string TypeOfMessage, std::string HTTPCode, std::string ContentType);

    std::string GetCoverImage(std::string ImageFormat, bool Base64);

    bool ConnectedHTTP = false;
    bool ConnectedSocket = false;
    bool MulticastRunning = false;
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

    UnityEngine::Texture2D* coverTexture = nullptr;

    std::string coverImageBase64 = "";
    std::string coverImageBase64PNG = "";
    // WARNING: These std::string types contain raw byte data, and will not make sense as text
    std::string coverImageJPG = "";
    std::string coverImagePNG = "";

    int players = 0;
    int maxPlayers = 0;
    std::string mpGameId = "";
    bool mpGameIdShown = false;

    std::string localIP = "127.0.0.1";
    std::string localIPv6 = "::1";
    std::string headsetType = "Unknown Android";

    //LoggerContextObject HTTPLogger = getLogger().WithContext("Server").WithContext("HTTP");
    //LoggerContextObject SocketLogger = getLogger().WithContext("Server").WithContext("Socket");
    //LoggerContextObject MulticastLogger = getLogger().WithContext("Server").WithContext("Multicast") ;

    //STManager(Logger& logger, LoggerContextObject& HTTPLogger, LoggerContextObject& SocketLogger, LoggerContextObject& MulticastLogger);
    STManager(Logger& logger);
};