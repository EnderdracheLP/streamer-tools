#define RAPIDJSON_HAS_STDSTRING 1 // Enable rapidjson's support for std::string
#define NO_CODEGEN_USE
#include <thread>
#include <cmath>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <unistd.h>
#include <mutex>
#include <fstream>
#include <sstream>
#include <iomanip>

#include "presencemanager.hpp"

#include "beatsaber-hook/shared/config/config-utils.hpp"
#include "GlobalNamespace/StandardLevelDetailView.hpp"

#define ADDRESS "0.0.0.0" // Binding to localhost
#define PORT 3500
#define CONNECTION_QUEUE_LENGTH 1 // How many connections to store to process

PresenceManager::PresenceManager(Logger& logger, const ConfigDocument& config) : logger(logger), config(config) {
    logger.info("Starting network thread . . .");
    networkThread = std::thread(&PresenceManager::runServer, this); // Run the thread
}

void PresenceManager::threadEntrypoint()    {
    logger.info("Starting server");
    if(!runServer()) {
        logger.error("Failed to run server!");
    }
}

// Creates the JSON to send back to MadMagic's application
std::string PresenceManager::constructResponse() {
    std::string configSection;
    if(playingTutorial) {
        configSection = "tutorialPresence";
    }   else if(playingCampaign) {
        configSection = "missionLevelPresence";
    }   else if(playingLevel.has_value())   {
        if(multiplayerLobby == std::nullopt) {
            // Choose either the practice or standard presence
            if(isPractice) {
                configSection = "practicePresence";
            }   else    {
                configSection = "standardLevelPresence";
            }
        }   else    {
            configSection = "multiplayerLevelPresence";
        }
    }   else if(multiplayerLobby != std::nullopt) {
        configSection = "multiplayerLobbyPresence";
    }   else    {
        configSection = "menuPresence";
    }
    logger.info("Using " + configSection + " config section.");

    bool didStatusTypeChange = configSection != previousSectionUsed;
    previousSectionUsed = configSection;

    rapidjson::Document doc;
    auto& alloc = doc.GetAllocator();
    doc.SetObject();

    statusLock.lock(); // Lock the mutex so that stuff doesn't get overwritten while we're reading from it
    std::string details = handlePlaceholders(config[configSection.c_str()]["details"].GetString());
    doc.AddMember("details", details, alloc);

    std::string state = handlePlaceholders(config[configSection.c_str()]["state"].GetString());
    doc.AddMember("state", state, alloc);
    statusLock.unlock();

    if(playingCampaign || playingLevel.has_value()) {
        if(!paused) {
            doc.AddMember("remaining", timeLeft, alloc);
        }
    }   else if((configSection == "menuPresence" || configSection == "multiplayerLobbyPresence") && !didStatusTypeChange) { // We have to set elapsed to false temporarily to tell the app that we want to reset the time
        doc.AddMember("elapsed", true, alloc);
    }

    doc.AddMember("largeImageKey", "image", alloc);
    doc.AddMember("smallImageKey", "quest", alloc);

    // Convert the document into a string
    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    doc.Accept(writer);
    return buffer.GetString();
}

// Replaces all of the placeholders in config.json
std::string PresenceManager::handlePlaceholders(std::string str) {
    // If we're playing a level, replace the level placeholders
    if(playingLevel.has_value()) {
        replaceAll(str, "{mapName}", playingLevel->name);
        // Use the Song author instead of the level author for levels that don't have one
        if(playingLevel->levelAuthor == "") {
            replaceAll(str, "{mapAuthor}", playingLevel->songAuthor);
        }   else    {
            replaceAll(str, "{mapAuthor}", playingLevel->levelAuthor);
        }
        
        replaceAll(str, "{songAuthor}", playingLevel->songAuthor);
        replaceAll(str, "{mapDifficulty}", playingLevel->selectedDifficulty);
    }

    // If we're in a lobby, when can emplace additional info
    if(multiplayerLobby.has_value()) {
        replaceAll(str, "{numPlayers}", std::to_string(multiplayerLobby->numberOfPlayers));
        replaceAll(str, "{numOthers}", std::to_string(multiplayerLobby->numberOfPlayers - 1)); // Subtract 1 for the number of others
        replaceAll(str, "{maxPlayers}", std::to_string(multiplayerLobby->maxPlayers));
    }

    if(paused)  {
        replaceAll(str, "{paused?}", "(Paused)");
    }   else    {
        replaceAll(str, "{paused?}", "");
    }
    

    return str;
}

// Replaces all occurunces of the key with the replacement inside of str
std::string PresenceManager::replaceAll(std::string& str, std::string key, std::string replacement) {
    int pos = str.find(key);
    while(pos != std::string::npos) {
        str.replace(pos, key.size(), replacement);
        pos = str.find(key, pos + replacement.size());
    }
    return str;
}

bool PresenceManager::runServer()   {
    // Make our IPv4 endpoint
    sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(PORT);
    addr.sin_addr.s_addr = inet_addr(ADDRESS);

    int sock = socket (AF_INET, SOCK_STREAM, IPPROTO_TCP); // Create the socket
    // Prevents the socket taking a while to close from causing a crash
    int iSetOption = 1;
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (char*)&iSetOption,sizeof(iSetOption));
    if(sock == -1)    {
        logger.error("Error creating socket: %s", strerror(errno));
        return false;
    }

    // Attempt to bind to our port
    if (bind(sock, (struct sockaddr*) &addr, sizeof(addr))) {
        logger.error("Error binding to port %d: %s", PORT, strerror(errno));
        close(sock);
        return false;
    }

    logger.info("Listening on port %d", PORT);
    while(true) {
        if (listen(sock, CONNECTION_QUEUE_LENGTH) == -1) { // Return if an error occurs listening for a request
            logger.error("Error listening for request");
            continue;
        }

        socklen_t socklen = sizeof addr;
        int client_sock = accept(sock, (struct sockaddr*) &addr, &socklen); // Accept the next request
        if(client_sock == -1)   {
            logger.error("Error accepting request");
            continue;
        }        

        logger.info("Client connected with address: %s", inet_ntoa(addr.sin_addr));
    
        std::string response = constructResponse();
        logger.info("Response: %s", response.c_str());

        int convertedLength = htonl(response.length());
        if(write(client_sock, &convertedLength, 4) == -1)    { // First send the length of the data
            logger.error("Error sending length prefix: %s", strerror(errno));
            close(client_sock); continue;
        }
        if(write(client_sock, response.c_str(), response.length()) == -1)    { // Then send the string
            logger.error("Error sending JSON: %s", strerror(errno));
            close(client_sock); continue;
        }

        close(client_sock); // Close the client's socket to avoid leaking resources
    }
    
    close(sock);
    return true;
}