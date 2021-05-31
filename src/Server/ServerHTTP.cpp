#define RAPIDJSON_HAS_STDSTRING 1 // Enable rapidjson's support for std::string
#define NO_CODEGEN_USE
#include "ServerHeaders.hpp"
#include "STmanager.hpp"
#include "Config.hpp"

//typedef struct { char* name, * value; } header_t;
//static header_t reqhdr[17] = { {"\0", "\0"} };

//LoggerContextObject HTTPLogger;

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

        ConnectedHTTP = true; // Set this to true here so it no longer sends out after a connection has first been established.

        // Pass the socket handle over and start a seperate thread for sending back the reply
        std::thread RequestHTTPThread([this, client_sock]() { return STManager::HandleRequestHTTP(client_sock); }); // Use threads for the response

        RequestHTTPThread.detach(); // Detach the thread from this thread
    }
    // RequestHTTPThread.join();
    close(sockHTTP);
    return true;
}

//char* request_header(const char* name)
//{
//    header_t* h = reqhdr;
//    while (h->name) {
//        if (strcmp(h->name, name) == 0) return h->value;
//        h++;
//    }
//    return NULL;
//}

// This assumes buffer is at least x bytes long,
// and that the socket is blocking.
void STManager::ReadXBytes(int socket, unsigned int x, char* buffer)
{
    int bytesRead = 0;
    int result;
    bool loaded = false;
    while (bytesRead < x)   // TODO: Check if we even need this loop
    {
        result = read(socket, buffer + bytesRead, x - bytesRead);
        if (result < 1)
        {
            logger.error("HTTP: Error receiving request: %s", strerror(errno));
            ConnectedHTTP = false;
            break;
        }
        if (bytesRead == result) loaded = true;

        bytesRead += result;
        LOG_DEBUG_HTTP("Received bytes: %d", bytesRead);
        LOG_DEBUG_HTTP("Received message: \n%s", buffer);

        std::string resultStr(buffer);
        if (((resultStr.find("\r\n\r\n") != std::string::npos) && (resultStr.find("GET") != std::string::npos)) || loaded) {
            ConnectedHTTP = false; 
            break;
        }
    }
}

void STManager::HandleRequestHTTP(int client_sock) {
    unsigned int length = 4096+1;
    char* buffer = 0;
    //std::string cppBuffer;
    std::string response;
    std::string messageStr;
    //// we assume that sizeof(length) will return 4 here.
    //ReadXBytes(client_sock, sizeof(length), (void*)(&length));
    buffer = new char[length];
    // TODO: This shouldn't be a seperate thread
    std::thread ReadXBytesThread([this, client_sock, length, buffer]() { return STManager::ReadXBytes(client_sock, length, buffer); }); // Use threads to read request
    //std::thread ReadXBytesThread([this, client_sock, length, cppBuffer]() { return STManager::ReadXBytes(client_sock, length, cppBuffer); }); // Use threads to read request
    //ReadXBytes(client_sock, length, buffer);

    //logger.debug((char*)buffer);
    ReadXBytesThread.join();
    std::string resultStr(buffer);
    delete[] buffer;
    #define ROUTE_START()       if ((resultStr.find("GET / ") != std::string::npos) || (resultStr.find("GET /index") != std::string::npos))
    #define ROUTE(METHOD,URI)    else if ((resultStr.find(METHOD " " URI " ") != std::string::npos))
    #define ROUTE_GET(URI)      ROUTE("GET", URI)
    #define ROUTE_POST(URI)      ROUTE("POST", URI)
    ROUTE_START() {
        messageStr = constructResponse();
        response =  "HTTP/1.1 200 OK\r\n" \
                    "Content-Length: " + std::to_string(messageStr.length()) + "\r\n" \
                    "Content-Type: application/json\r\n" \
                    "Access-Control-Allow-Origin: *\r\n" \
                    "X-Powered-By: " + STModInfo.id + "/" + STModInfo.version + "\r\n\r\n" + \
                    messageStr;
    }
    ROUTE_GET("/cover/base64/") goto COVER; // I know eww goto, but give me a better solution
    ROUTE_GET("/cover/base64") {
        COVER:
        std::string stats = STManager::coverImageBase64;
        if (stats.empty()) goto NotFound;
        response =  "HTTP/1.1 200 OK\r\n"\
                    "Content-Length: " + std::to_string(stats.length()) + "\n"\
                    "Content-Type: text/plain\r\n"\
                    "Access-Control-Allow-Origin: *\r\n" \
                    "X-Powered-By: " + STModInfo.id + "/" + STModInfo.version + "\r\n\r\n" + \
                    "data:image/jpg;base64," + stats;
    }
    ROUTE_GET("/cover.jpg") {
        std::string stats = STManager::coverImageJPG;
        if (stats.empty()) goto NotFound;
        response =  "HTTP/1.1 200 OK\r\n"\
                    "Content-Length: " + std::to_string(stats.length()) + "\n"\
                    "Content-Type: image/jpg\r\n"\
                    "Access-Control-Allow-Origin: *\r\n" \
                    "X-Powered-By: " + STModInfo.id + "/" + STModInfo.version + "\r\n\r\n" + \
                    stats;
    }
    ROUTE_GET("/config/") goto CONFIG; // I know eww goto, but give me a better solution
    ROUTE_GET("/config") {
        CONFIG:
        std::string stats = constructConfigResponse();
        if (stats.empty()) goto NotFound;
        response =  "HTTP/1.1 200 OK\r\n"\
                    "Content-Length: " + std::to_string(stats.length()) + "\n"\
                    "Content-Type: application/json\r\n"\
                    "Access-Control-Allow-Origin: *\r\n" \
                    "X-Powered-By: " + STModInfo.id + "/" + STModInfo.version + "\r\n\r\n" + \
                    stats;
    }
    ROUTE_POST("/config/") goto PCONFIG; // I know eww goto, but give me a better solution
    ROUTE_POST("/config") {
        PCONFIG:
        size_t start = resultStr.find("{");
        LOG_DEBUG_HTTP("index of {: " + std::to_string(start));
        size_t end = resultStr.find("}");
        LOG_DEBUG_HTTP("index of }: " + std::to_string(end));
        std::string json = resultStr.substr(start,  end - start + 1);
        LOG_DEBUG_HTTP("json: " + json);
        rapidjson::Document document;
        document.Parse(json);
        LOG_DEBUG_HTTP("decimals: " + std::to_string(document["decimals"].GetInt()));
        LOG_DEBUG_HTTP("dontenergy: " + std::to_string(document["dontenergy"].GetBool()));
        LOG_DEBUG_HTTP("dontmpcode: " + std::to_string(document["dontmpcode"].GetBool()));
        LOG_DEBUG_HTTP("alwaysmpcode: " + std::to_string(document["alwaysmpcode"].GetBool()));
        LOG_DEBUG_HTTP("alwaysupdate: " + std::to_string(document["alwaysupdate"].GetBool()));
        getModConfig().DecimalsForNumbers.SetValue(document["decimals"].GetInt());
        getModConfig().DontEnergy.SetValue(document["dontenergy"].GetBool());
        getModConfig().DontMpCode.SetValue(document["dontmpcode"].GetBool());
        getModConfig().AlwaysMpCode.SetValue(document["alwaysmpcode"].GetBool());
        getModConfig().AlwaysUpdate.SetValue(document["alwaysupdate"].GetBool());
    }
    else {
        // 404 or invalid
        NotFound:
        messageStr =    "<!DOCTYPE html> "\
                        "<html> "\
                        "<head> <title>streamer-tools - 404 Not found</title> "\
                        "<link href='https://fonts.googleapis.com/css?family=Open+Sans:400,400italic,700,700italic' rel='stylesheet' type='text/css'> "\
                        "<style> body { color: #EEEEEE; background-color: #202225; font-size: 14px; font-family: 'Open Sans'; } </style> "\
                        "</head> "\
                        "<body> <div style=\"font-size: 30px;\">Streamer-tools - 404 Not found</div> "\
                        "<div style=\"color: #888; margin-bottom: 10px; padding-left: 20px;\">The endpoint you were looking for could not be found.</div> "\
                        "<div style=\"font-size: 18px; margin-top: 30px; border-top: solid #BBBBBB 2px; padding: 10px; width: fit-content;\"><i>" + STModInfo.id + "/" + STModInfo.version + " ("+ headsetType +") server at " + STManager::localIp + ":" + std::to_string(PORT_HTTP) + "</i></div> "\
                        "</body> </html>"; // Yes this is long but page is pretty-ish
        response =  "HTTP/1.1 404 Not Found\n"\
                    "Content-Length: " + std::to_string(messageStr.length()) + "\n"\
                    "Access-Control-Allow-Origin: *\r\n" \
                    "X-Powered-By: " + STModInfo.id + "/" + STModInfo.version + "\r\n\r\n" + \
                    messageStr;
    }
    if (!response.empty()) {
        SendResponse: // Just incase we ever need to use goto SendRequest to skip over code
        int convertedLength = htonl(response.length());
        if (write(client_sock, response.c_str(), response.length()) == -1) { // Then send the string
            logger.error("HTTP: Error sending Response: \n%s", strerror(errno));
            ConnectedHTTP = false;
            close(client_sock); return;
        }
        LOG_DEBUG_HTTP("HTTP Response: \n%s", response.c_str());
    }
    ConnectedHTTP = false;
    close(client_sock); // Close the client's socket to avoid leaking resources
    return;
}