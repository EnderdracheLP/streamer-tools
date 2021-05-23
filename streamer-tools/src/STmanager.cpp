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

#include "STmanager.hpp"

#include "beatsaber-hook/shared/config/config-utils.hpp"
#include "GlobalNamespace/StandardLevelDetailView.hpp"

#define ADDRESS "0.0.0.0" // Binding to localhost
#define PORT 3501
#define PORT_HTTP 3502
#define CONNECTION_QUEUE_LENGTH 1 // How many connections to store to process

STManager::STManager(Logger& logger, const ConfigDocument& config) : logger(logger), config(config) {
    logger.info("Starting network thread . . .");
    networkThread = std::thread(&STManager::runServer, this); // Run the thread
    networkThreadHTTP = std::thread(&STManager::runServerHTTP, this); // Run the thread
}

void STManager::threadEntrypoint()    {
    logger.info("Starting server");
    if(!runServer()) {
        logger.error("Failed to run server!");
    }
    if (!runServerHTTP()) {
        logger.error("Failed to run HTTP server!");
    }
}

// Creates the JSON to send back to the Streamer Tools application
std::string STManager::constructResponse() {
    rapidjson::Document doc;
    auto& alloc = doc.GetAllocator();
    doc.SetObject();

    statusLock.lock(); // Lock the mutex so that stuff doesn't get overwritten while we're reading from it
    
    doc.AddMember("type", STManager::type, alloc);
    doc.AddMember("isPractice", STManager::isPractice, alloc);
    doc.AddMember("paused", STManager::paused, alloc);
    doc.AddMember("time", STManager::time, alloc);
    doc.AddMember("endTime", STManager::endTime, alloc);
    doc.AddMember("score", STManager::score, alloc);
    doc.AddMember("rank", STManager::rank, alloc);
    doc.AddMember("combo", STManager::combo, alloc);
    doc.AddMember("energy", STManager::energy, alloc);
    doc.AddMember("accuracy", STManager::accuracy, alloc);
    doc.AddMember("levelName", STManager::levelName, alloc);
    doc.AddMember("levelSubName", STManager::levelSubName, alloc);
    doc.AddMember("levelAuthor", STManager::levelAuthor, alloc);
    doc.AddMember("songAuthor", STManager::songAuthor, alloc);
    doc.AddMember("id", STManager::id, alloc);
    doc.AddMember("difficulty", STManager::difficulty, alloc);
    doc.AddMember("bpm", STManager::bpm, alloc);
    doc.AddMember("njs", STManager::njs, alloc);
    doc.AddMember("players", STManager::players, alloc);
    doc.AddMember("maxPlayers", STManager::maxPlayers, alloc);
    statusLock.unlock();

    // Convert the document into a string
    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    doc.Accept(writer);
    return buffer.GetString();
}

bool STManager::runServer()   {
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

bool STManager::runServerHTTP() {
    // Make our IPv4 endpoint
    sockaddr_in addrHTTP;
    addrHTTP.sin_family = AF_INET;
    addrHTTP.sin_port = htons(PORT_HTTP);
    addrHTTP.sin_addr.s_addr = inet_addr(ADDRESS);

    int sockHTTP = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP); // Create the socket
    // Prevents the socket taking a while to close from causing a crash
    int iSetOption = 1;
    setsockopt(sockHTTP, SOL_SOCKET, SO_REUSEADDR, (char*)&iSetOption, sizeof(iSetOption));
    if (sockHTTP == -1) {
        logger.error("HTTP: Error creating socket: %s", strerror(errno));
        return false;
    }

    // Attempt to bind to our port
    if (bind(sockHTTP, (struct sockaddr*)&addrHTTP, sizeof(addrHTTP))) {
        logger.error("HTTP: Error binding to port %d: %s", PORT_HTTP, strerror(errno));
        close(sockHTTP);
        return false;
    }

    logger.info("HTTP: Listening on port %d", PORT_HTTP);
    while (true) {
        if (listen(sockHTTP, CONNECTION_QUEUE_LENGTH) == -1) { // Return if an error occurs listening for a request
            logger.error("HTTP: Error listening for request");
            continue;
        }

        socklen_t socklenHTTP = sizeof addrHTTP;
        int client_sock = accept(sockHTTP, (struct sockaddr*)&addrHTTP, &socklenHTTP); // Accept the next request
        if (client_sock == -1) {
            logger.error("HTTP: Error accepting request");
            continue;
        }

        logger.info("HTTP: Client connected with address: %s", inet_ntoa(addrHTTP.sin_addr));

        std::string stats = constructResponse();
        std::string response = "HTTP/1.1 200 OK\nContent-Length: " + std::to_string(stats.length()) + "\nContent-Type: application/json\nAccess-Control-Allow-Origin: *\n\n" + stats;
        logger.info("HTTP Response: %s", response.c_str());

        int convertedLength = htonl(response.length());
        if (write(client_sock, response.c_str(), response.length()) == -1) { // Then send the string
            logger.error("HTTP: Error sending JSON: %s", strerror(errno));
            close(client_sock); continue;
        }

        close(client_sock); // Close the client's socket to avoid leaking resources
    }

    close(sockHTTP);
    return true;
}