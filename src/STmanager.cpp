#define RAPIDJSON_HAS_STDSTRING 1 // Enable rapidjson's support for std::string
#define NO_CODEGEN_USE
#include <thread>
#include <mutex>

#include "STmanager.hpp"
#include "Config.hpp"

#include "GlobalNamespace/StandardLevelDetailView.hpp"

#define ADDRESS "0.0.0.0" // Binding to localhost
#define ADDRESS_MULTI "232.0.53.5"  // Testing setting was "225.1.1.1" and "224.0.0.1" which sends to all hosts on the network
#define PORT_MULTI 53500
#define PORT 53501
#define PORT_HTTP 53502
#define CONNECTION_QUEUE_LENGTH 2 // How many connections to store to process

STManager::STManager(Logger& logger) : logger(logger) {
    logger.info("Starting network thread . . .");
    networkThread = std::thread(&STManager::runServer, this); // Run the thread
    networkThreadHTTP = std::thread(&STManager::runServerHTTP, this); // Run the thread
    networkThreadMulticast = std::thread(&STManager::MulticastServer, this); // Run the thread
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

std::string STManager::constructConfigResponse() {
    rapidjson::Document doc;
    auto& alloc = doc.GetAllocator();
    doc.SetObject();

    doc.AddMember("decimals", getModConfig().DecimalsForNumbers.GetValue(), alloc);
    doc.AddMember("dontenergy", getModConfig().DontEnergy.GetValue(), alloc);
    doc.AddMember("dontmpcode", getModConfig().DontMpCode.GetValue(), alloc);
    doc.AddMember("alwaysmpcode", getModConfig().AlwaysMpCode.GetValue(), alloc);
    doc.AddMember("alwaysupdate", getModConfig().AlwaysUpdate.GetValue(), alloc);

    // Convert the document into a string
    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    doc.Accept(writer);
    return buffer.GetString();
}

std::string STManager::constructCoverResponse() {
    rapidjson::Document doc;
    auto& alloc = doc.GetAllocator();
    doc.SetObject();
    statusLock.lock();
    doc.AddMember("coverImageBase64", STManager::coverImageBase64, alloc);
    statusLock.unlock();

    // Convert the document into a string
    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    doc.Accept(writer);
    return buffer.GetString();
}