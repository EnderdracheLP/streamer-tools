#define RAPIDJSON_HAS_STDSTRING 1 // Enable rapidjson's support for std::string
#define NO_CODEGEN_USE
#include <thread>
#include <mutex>

#include "STmanager.hpp"
#include "Config.hpp"
#include "Server/ServerHeaders.hpp"

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
    doc.AddMember("configFetched", configFetched, alloc);
    doc.AddMember("coverFetchable", STManager::coverFetchable, alloc);
    statusLock.unlock();

    // Convert the document into a string
    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    doc.Accept(writer);
    return buffer.GetString();
}

std::string STManager::multicastResponse() {
    rapidjson::Document doc;
    auto& alloc = doc.GetAllocator();
    doc.SetObject();

    std::string http = localIP + ":" + std::to_string(PORT_HTTP);
    std::string httpv6 = "[" + localIPv6 + "]:" + std::to_string(PORT_HTTP);
    std::string socket = localIP + ":" + std::to_string(PORT);
    std::string socketv6 = "[" + localIPv6 + "]:" + std::to_string(PORT);

    doc.AddMember("ModID", STModInfo.id, alloc);
    doc.AddMember("ModVersion", STModInfo.version, alloc);
    doc.AddMember("Socket", socket, alloc);
    doc.AddMember("HTTP", http, alloc);
    doc.AddMember("Socketv6", socketv6, alloc);
    doc.AddMember("HTTPv6", httpv6, alloc);
    doc.AddMember("GameVersion", gameVersion, alloc);

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

    doc.AddMember("lastChanged", getModConfig().LastChanged.GetValue(), alloc);
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

std::string STManager::constructPositionResponse() {
    rapidjson::Document doc;
    auto& alloc = doc.GetAllocator();
    doc.SetObject();
    if (Head) {
        UnityEngine::Vector3 HeadVectPos = Head->get_position();
        rapidjson::Value HeadPos(rapidjson::kArrayType);
        HeadPos.PushBack(HeadVectPos.x, alloc).PushBack(HeadVectPos.y, alloc).PushBack(HeadVectPos.z, alloc);
        doc.AddMember("HeadPos", HeadPos, alloc);

        UnityEngine::Vector3 HeadVectRot = Head->get_eulerAngles();
        rapidjson::Value HeadRot(rapidjson::kArrayType);
        HeadRot.PushBack(HeadVectRot.x, alloc).PushBack(HeadVectRot.y, alloc).PushBack(HeadVectRot.z, alloc);
        doc.AddMember("HeadRot", HeadRot, alloc);
    }

    if (VR_Right) {
        UnityEngine::Vector3 VRC_RightVectPos = VR_Right->get_position();
        rapidjson::Value VRC_RightPos(rapidjson::kArrayType);
        VRC_RightPos.PushBack(VRC_RightVectPos.x, alloc).PushBack(VRC_RightVectPos.y, alloc).PushBack(VRC_RightVectPos.z, alloc);
        doc.AddMember("VR_RightPos", VRC_RightPos, alloc);

        UnityEngine::Vector3 VRC_RightVectRot = VR_Right->get_eulerAngles();
        rapidjson::Value VRC_RightRot(rapidjson::kArrayType);
        VRC_RightRot.PushBack(VRC_RightVectRot.x, alloc).PushBack(VRC_RightVectRot.y, alloc).PushBack(VRC_RightVectRot.z, alloc);
        doc.AddMember("VR_RightRot", VRC_RightRot, alloc);
    }

    if (VR_Left) {
        UnityEngine::Vector3 VRC_LeftVectPos = VR_Left->get_position();
        rapidjson::Value VRC_LeftPos(rapidjson::kArrayType);
        VRC_LeftPos.PushBack(VRC_LeftVectPos.x, alloc).PushBack(VRC_LeftVectPos.y, alloc).PushBack(VRC_LeftVectPos.z, alloc);
        doc.AddMember("VR_LeftPos", VRC_LeftPos, alloc);

        UnityEngine::Vector3 VRC_LeftVectRot = VR_Left->get_eulerAngles();
        rapidjson::Value VRC_LeftRot(rapidjson::kArrayType);
        VRC_LeftRot.PushBack(VRC_LeftVectRot.x, alloc).PushBack(VRC_LeftVectRot.y, alloc).PushBack(VRC_LeftVectRot.z, alloc);
        doc.AddMember("VR_LeftRot", VRC_LeftRot, alloc);
    }

    // Convert the document into a string
    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    doc.Accept(writer);
    return buffer.GetString();
}