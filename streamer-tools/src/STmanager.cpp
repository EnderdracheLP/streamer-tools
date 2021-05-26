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

#define ADDRESS "0.0.0.0" // Binding to localhost
#define ADDRESS_MULTI "232.0.53.5"  // Testing setting was "225.1.1.1" and "224.0.0.1" which sends to all hosts on the network
#define PORT_MULTI 53500
#define PORT 53501
#define PORT_HTTP 53502
#define CONNECTION_QUEUE_LENGTH 2 // How many connections to store to process

bool Connected = false;

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
        
        Connected = true; // Set this to true here so it no longer sends out after a connection has been established first.

        // Pass the socket handle over and start a seperate thread for sending back the reply
        std::thread RequestThread([this, client_sock]() { return STManager::sendRequest(client_sock); }); // Use threads for the response
        RequestThread.detach(); // Detach the thread from this thread


        //std::string response = constructResponse();
        //logger.info("Response: %s", response.c_str());

        //int convertedLength = htonl(response.length());
        //if(write(client_sock, &convertedLength, 4) == -1)    { // First send the length of the data
        //    logger.error("Error sending length prefix: %s", strerror(errno));
        //    close(client_sock); continue;
        //}
        //if(write(client_sock, response.c_str(), response.length()) == -1)    { // Then send the string
        //    logger.error("Error sending JSON: %s", strerror(errno));
        //    close(client_sock); continue;
        //}

        //close(client_sock); // Close the client's socket to avoid leaking resources
        //std::chrono::milliseconds timespan(50);
        //std::this_thread::sleep_for(timespan);
    }
    
    close(sock);
    return true;
}

void STManager::sendRequest(int client_sock) {

    while (client_sock != -1) {
        std::string response = constructResponse();
        logger.info("Response: %s", response.c_str());

        int convertedLength = htonl(response.length());
        if (write(client_sock, &convertedLength, 4) == -1) { // First send the length of the data
            logger.error("Error sending length prefix: %s", strerror(errno));
            close(client_sock); return;
        }
        if (write(client_sock, response.c_str(), response.length()) == -1) { // Then send the string
            logger.error("Error sending JSON: %s", strerror(errno));
            close(client_sock); return;
        }
        std::chrono::milliseconds timespan(100);
        std::this_thread::sleep_for(timespan);
    }
    close(client_sock); // Close the client's socket to avoid leaking resources
    return;
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

    //std::thread RequestHTTPThread;

    logger.info("HTTP: Listening on port %d", PORT_HTTP);
    while (true) {
        if (listen(sockHTTP, CONNECTION_QUEUE_LENGTH) == -1) { // Return if an error occurs listening for a request
            logger.error("HTTP: Error listening for request");
            continue;
        }

        socklen_t socklenHTTP = sizeof addrHTTP;
        int client_sock = accept(sockHTTP, (struct sockaddr*)&addrHTTP, &socklenHTTP); // Accept the next request
        if (client_sock == -1) {
            logger.error("HTTP: Error accepting request %s", strerror(errno));
            continue;
        }


        logger.info("HTTP: Client connected with address: %s", inet_ntoa(addrHTTP.sin_addr));

        Connected = true; // Set this to true here so it no longer sends out after a connection has first been established.

        // Pass the socket handle over and start a seperate thread for sending back the reply
        std::thread RequestHTTPThread([this, client_sock]() { return STManager::HandleRequestHTTP(client_sock); }); // Use threads for the response

        RequestHTTPThread.detach(); // Detach the thread from this thread

        //std::string stats = constructResponse();
        //std::string response = "HTTP/1.1 200 OK\nContent-Length: " + std::to_string(stats.length()) + "\nContent-Type: application/json\nAccess-Control-Allow-Origin: *\n\n" + stats;
        //logger.info("HTTP Response: %s", response.c_str());

        //int convertedLength = htonl(response.length());
        //if (write(client_sock, response.c_str(), response.length()) == -1) { // Then send the string
        //    logger.error("HTTP: Error sending JSON: %s", strerror(errno));
        //    close(client_sock); continue;
        //}

        //close(client_sock); // Close the client's socket to avoid leaking resources
        //std::chrono::milliseconds timespan(50);
        //std::this_thread::sleep_for(timespan);
    }
    // RequestHTTPThread.join();
    close(sockHTTP);
    return true;
}

// This assumes buffer is at least x bytes long,
// and that the socket is blocking.
void STManager::ReadXBytes(int socket, unsigned int x, char* buffer)
{
    int bytesRead = 0;
    int result;
    while (bytesRead < x)
    {
        result = read(socket, buffer + bytesRead, x - bytesRead);
        if (result < 1)
        {
            logger.error("HTTP: Error receiving request: %s", strerror(errno));
        }

        bytesRead += result;
        logger.debug("Received bytes: %d", bytesRead);
        logger.debug("Received message: \n%s", buffer);
    }
}

void STManager::HandleRequestHTTP(int client_sock) {

    unsigned int length = 350;
    char* buffer = 0;
    //// we assume that sizeof(length) will return 4 here.
    //ReadXBytes(client_sock, sizeof(length), (void*)(&length));
    buffer = new char[length];
    ReadXBytes(client_sock, length, buffer);

    logger.debug(buffer);

    std::string resultStr(buffer);
    delete[] buffer;
    bool invalid = resultStr.find("GET / ") == std::string::npos;
    if (invalid) {
        std::string messageError = ("<!DOCTYPE HTML PUBLIC \" -\/\/IETF//DTD HTML 2.0//EN\">\n<html><head>\n<title>404 Not Found</title>\n</head><body>\n<h1>Not Found</h1>\n<p>The requested URL was not found on this server.</p>\n<hr>\n<address>" + STModInfo.id + "/" + STModInfo.version + " (Oculus Quest) Server at Port " + std::to_string(PORT_HTTP) + "</address>\n</body></html>");
        std::string responseInvalid = "HTTP/1.1 404 Not Found\nContent-Length: " + std::to_string(messageError.length()) + "\nContent-Type: text/html\nAccess-Control-Allow-Origin: *\n\n" + messageError;
        logger.info("HTTP Not Found Response: %s", responseInvalid.c_str());

        int convertedLengthInvalid = htonl(responseInvalid.length());
        if (write(client_sock, responseInvalid.c_str(), responseInvalid.length()) == -1) { // Then send the string
            logger.error("HTTP: Error sending HTTP Response: %s", strerror(errno));
            close(client_sock); logger.debug("Received message: \n%s", resultStr.c_str()); return;
        }
        logger.error("HTTP: Response Status Code: %s", strerror(errno));
        close(client_sock); // Close the client's socket to avoid leaking resources
        return;
    } else {
        std::string stats = constructResponse();
        std::string response = "HTTP/1.1 200 OK\nContent-Length: " + std::to_string(stats.length()) + "\nContent-Type: application/json\nAccess-Control-Allow-Origin: *\n\n" + stats;
        logger.info("HTTP Response: \n%s", response.c_str());

        int convertedLength = htonl(response.length());
        if (write(client_sock, response.c_str(), response.length()) == -1) { // Then send the string
            logger.error("HTTP: Error sending JSON: \n%s", strerror(errno));
            close(client_sock); return;
        }
    }
    close(client_sock); // Close the client's socket to avoid leaking resources
    return;
}

bool STManager::MulticastServer() {
    struct in_addr        localInterface;
    struct sockaddr_in    groupSock;
    int                   datalen;
    char                  databuf[1024];
    
    /*
    * Create a datagram socket on which to send.
    */
    int sd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sd < 0) {
        logger.error("opening datagram socket");
        return false;
    }

    /*
     * Initialize the group sockaddr structure with a
     * group address of 225.1.1.1 and port 5555.
     */
    memset((char*)&groupSock, 0, sizeof(groupSock));
    groupSock.sin_family = AF_INET;
    groupSock.sin_addr.s_addr = inet_addr(ADDRESS_MULTI);
    groupSock.sin_port = htons(PORT_MULTI);
    logger.info("Initializing Multicast on %s:%d", ADDRESS_MULTI, PORT_MULTI);

    /*
     * Disable loopback so you do not receive your own datagrams.
     */
    {
        char loopch = 0;

        if (setsockopt(sd, IPPROTO_IP, IP_MULTICAST_LOOP,
            (char*)&loopch, sizeof(loopch)) < 0) {
            logger.error("setting IP_MULTICAST_LOOP: %s", strerror(errno));
            close(sd);
            return false;
        }
    }

    /*
     * Set local interface for outbound multicast datagrams.
     * The IP address specified must be associated with a local,
     * multicast-capable interface.
     */
    localInterface.s_addr = inet_addr(ADDRESS);
    if (setsockopt(sd, IPPROTO_IP, IP_MULTICAST_IF,
        (char*)&localInterface,
        sizeof(localInterface)) < 0) {
        logger.error("setting local interface: %s", strerror(errno));
        return false;
    }

        struct ifreq ifr;

        while (!Connected) {
                std::chrono::milliseconds timespan(30000);

                /* I want to get an IPv4 IP address */
                ifr.ifr_addr.sa_family = AF_INET;

                /* I want IP address attached to "wlan0" */
                strncpy(ifr.ifr_name, "wlan0", IFNAMSIZ - 1);

                ioctl(sd, SIOCGIFADDR, &ifr);

                /*
                * Create message to be sent
                */
                //    std::string message = inet_ntoa(((struct sockaddr_in*)&ifr.ifr_addr)->sin_addr);
                std::string ip = inet_ntoa(((struct sockaddr_in*)&ifr.ifr_addr)->sin_addr);
                std::string http = ip + ":" + std::to_string(PORT_HTTP);
                std::string socket = ip + ":" + std::to_string(PORT);
                std::string message = multicastResponse(socket, http);

                /*
                 * Send a message to the multicast group specified by the
                 * groupSock sockaddr structure.
                 */

                strcpy(databuf, message.c_str());
                datalen = message.length();
                if (sendto(sd, databuf, datalen, 0,
                    (struct sockaddr*)&groupSock,
                    sizeof(groupSock)) < 0)
                {
                    logger.error("Multicast: error sending datagram message: %s", strerror(errno));
                }
                std::this_thread::sleep_for(timespan);
    }
    return true;
}