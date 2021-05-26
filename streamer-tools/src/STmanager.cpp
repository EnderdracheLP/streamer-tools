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

#include <chrono>
#include <net/if.h>
#include <sys/ioctl.h>

#include "STmanager.hpp"

#include "beatsaber-hook/shared/config/config-utils.hpp"
#include "GlobalNamespace/StandardLevelDetailView.hpp"

STManager::STManager(Logger& logger, const ConfigDocument& config) : logger(logger), config(config) {
    logger.info("Starting network thread . . .");
    networkThread = std::thread(&STManager::runServer, this); // Run the thread
    networkThreadHTTP = std::thread(&STManager::runServerHTTP, this); // Run the thread
    networkThreadMulticast = std::thread(&STManager::MulticastServer, this); // Run the thread
}

void STManager::threadEntrypoint()    {
    logger.info("Starting server");
    if(!runServer()) {
        logger.error("Failed to run server!");
    }
    if (!runServerHTTP()) {
        logger.error("Failed to run HTTP server!");
    }
    if (!MulticastServer()) {
        logger.error("Failed to run Multicast server!");
    }
    else logger.info("Multicast Server finished connection established!");
}

// Creates the JSON to send back to the Streamer Tools application
std::string STManager::constructResponse() {

    statusLock.lock(); // Lock the mutex so that stuff doesn't get overwritten while we're reading from it

    rapidjson::Document doc;
    auto& alloc = doc.GetAllocator();
    doc.SetObject();
    
    doc.AddMember("location", STManager::location, alloc);
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
    doc.AddMember("mpGameId", STManager::mpGameId, alloc);
    doc.AddMember("mpGameIdShown", STManager::mpGameIdShown, alloc);
    doc.AddMember("goodCuts", STManager::goodCuts, alloc);
    doc.AddMember("badCuts", STManager::badCuts, alloc);
    doc.AddMember("missedNotes", STManager::missedNotes, alloc);
    doc.AddMember("fps", STManager::fps, alloc);
    statusLock.unlock();

    // Convert the document into a string
    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    doc.Accept(writer);
    return buffer.GetString();
}

std::string STManager::multicastResponse(std::string socket, std::string http) {
    rapidjson::Document doc;
    auto& alloc = doc.GetAllocator();
    doc.SetObject();

    doc.AddMember("ModID", STModInfo.id, alloc);
    doc.AddMember("ModVersion", STModInfo.version, alloc);
    doc.AddMember("Socket", socket, alloc);
    doc.AddMember("HTTP", http, alloc);

    // Convert the document into a string
    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    doc.Accept(writer);
    return buffer.GetString();
}