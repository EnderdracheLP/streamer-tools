#include <thread>
#include<cmath>
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

#define ADDRESS "0.0.0.0" // Binding to localhost
#define PORT 3500
#define CONNECTION_QUEUE_LENGTH 1 // How many connections to store to process

PresenceManager::PresenceManager(const Logger& logger, const ConfigDocument& config) : logger(logger), config(config) {
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
// Yes, I know I should be using rapidjson, but it was returning 'null' constantly for some reason
std::string PresenceManager::constructResponse() {
    statusLock.lock(); // Lock the mutex so that stuff doesn't get overwritten while we're reading from it
    std::string configSection;
    if(playingTutorial) {
        configSection = "tutorialPresence";
    }   else if(playingCampaign) {
        configSection = "missionLevelPresence";
    }   else if(playingLevel.has_value())   {
        configSection = "standardLevelPresence";
    }   else    {
        configSection = "menuPresence";
    }

    std::string details = escape_json(handlePlaceholders(config[configSection.c_str()]["details"].GetString()));
    std::string state = escape_json(handlePlaceholders(config[configSection.c_str()]["state"].GetString()));
    statusLock.unlock();
    logger.info("Details: %s. State: %s", details.c_str(), state.c_str());

    if((playingCampaign || playingLevel.has_value()) && !paused) {
        return "{\"details\": \"" + details + "\", \"state\": \"" + state + "\", \"largeImageKey\": \"image\", \"smallImageKey\": \"quest\", \"remaining\": " + std::to_string(timeLeft) + "}";
    }   else    {
        return "{\"details\": \"" + details + "\", \"state\": \"" + state + "\", \"largeImageKey\": \"image\", \"smallImageKey\": \"quest\"}";
    }
}

// Removes quotes and other disallowed characters from JSON
std::string PresenceManager::escape_json(const std::string &s) {
    std::ostringstream o;
    for (auto c = s.cbegin(); c != s.cend(); c++) {
        if (*c == '"' || *c == '\\' || ('\x00' <= *c && *c <= '\x1f')) {
            o << "\\u"
              << std::hex << std::setw(4) << std::setfill('0') << (int)*c;
        } else {
            o << *c;
        }
    }
    return o.str();
}

// Replaces all of the placeholders in config.json
std::string PresenceManager::handlePlaceholders(std::string str) {
    replaceAll(str, "{mapName}", playingLevel->name);
    // Use the Song author instead of the level author for levels that don't have one
    if(playingLevel->levelAuthor == "") {
        replaceAll(str, "{mapAuthor}", playingLevel->songAuthor);
    }   else    {
        replaceAll(str, "{mapAuthor}", playingLevel->levelAuthor);
    }
    
    replaceAll(str, "{songAuthor}", playingLevel->songAuthor);
    if(paused)  {
        replaceAll(str, "{paused?}", "(Paused)");
    }   else    {
        replaceAll(str, "{paused?}", "");
    }
    replaceAll(str, "{mapDifficulty}", playingLevel->selectedDifficulty);

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