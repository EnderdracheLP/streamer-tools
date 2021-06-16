#define RAPIDJSON_HAS_STDSTRING 1 // Enable rapidjson's support for std::string
#define NO_CODEGEN_USE
#include "ServerHeaders.hpp"
#include "STmanager.hpp"
#include "Config.hpp"

//LoggerContextObject HTTPLogger;

bool STManager::runServerHTTP() {
    // Make our IPv6 endpoint
    sockaddr_in6 ServerHTTP;
    ServerHTTP.sin6_family = AF_INET6;
    ServerHTTP.sin6_port = htons(PORT_HTTP);
    ServerHTTP.sin6_addr = in6addr_any;
    int v6OnlyEnabled = 0;
    char numeric_addr[INET6_ADDRSTRLEN];

    int sockHTTPv6 = socket(AF_INET6, SOCK_STREAM, IPPROTO_TCP); // Create the socket
    // Prevents the socket taking a while to close from causing a crash
    int iSetOption = 1;
    setsockopt(sockHTTPv6, SOL_SOCKET, SO_REUSEADDR, (char*)&iSetOption, sizeof(iSetOption));
    setsockopt(sockHTTPv6, IPPROTO_IPV6, IPV6_V6ONLY, &v6OnlyEnabled, sizeof(v6OnlyEnabled)); // Disable v6 Only restriction to allow v4 connections
    if (sockHTTPv6 == -1) {
        logger.error("HTTP: Error creating socket: %s", strerror(errno));
        return false;
    }

    // Attempt to bind to our port
    if (bind(sockHTTPv6, (struct sockaddr*)&ServerHTTP, sizeof(ServerHTTP))) {
        logger.error("HTTP: Error binding to port %d: %s", PORT_HTTP, strerror(errno));
        close(sockHTTPv6);
        return false;
    }

    logger.info("HTTP: Listening on port %d", PORT_HTTP);
    while (true) {
        if (listen(sockHTTPv6, CONNECTION_QUEUE_LENGTH) == -1) { // Return if an error occurs listening for a request
            logger.error("HTTP: Error listening for request");
            continue;
        }

        socklen_t socklenHTTP = sizeof ServerHTTP;
        int client_sock = accept(sockHTTPv6, (struct sockaddr*)&ServerHTTP, &socklenHTTP); // Accept the next request
        if (client_sock == -1) {
            logger.error("HTTP: Error accepting request %s", strerror(errno));
            continue;
        }
        // Getting the client_ip using inet_ntop
        std::string ClientIP(inet_ntop(AF_INET6, (struct sockaddr*)&ServerHTTP.sin6_addr, numeric_addr, sizeof numeric_addr));

        // If ClientIP starts with ::ffff: it's an IPv4, format IPv6 mapping to IPv4 address mapping
        if (ClientIP.starts_with("::ffff:")) {
            ClientIP = ClientIP.substr(7, ClientIP.length());
        }
        logger.info("HTTP: Client connected with address: %s", ClientIP.c_str());


        ConnectedHTTP = true; // Set this to true here so it no longer sends out Multicast after a connection has been established.

        // Pass the socket handle over and start a seperate thread for sending back the reply
        std::thread RequestHTTPThread([this, client_sock]() { return STManager::HandleRequestHTTP(client_sock); }); // Use threads for the response

        RequestHTTPThread.detach(); // Detach the thread from this thread
    }
    close(sockHTTPv6);
    return true;
}

// This assumes buffer is at least x bytes long,
// and that the socket is blocking.
void STManager::ReadXBytes(int socket, unsigned int x, char* buffer)
{
    std::string BufferStr(buffer);
    int bytesRead = 0;
    int result;
    bool loaded = false;

    while (bytesRead < x)
    {
        result = read(socket, buffer + bytesRead, x - bytesRead);
        if (result < 1)
        {
            logger.error("HTTP: Error receiving request: %s", strerror(errno));
            ConnectedHTTP = false;
            break;
        }
        bytesRead += result;
        LOG_DEBUG_HTTP("Received bytes: %d", bytesRead);
        LOG_DEBUG_HTTP("Received Request: \n%s", buffer);

        BufferStr = buffer;
        if ((BufferStr.find("Content-Length:") != std::string::npos)) {
            size_t start = BufferStr.find("Content-Length: ");
            //LOG_DEBUG_HTTP("index of Content-Length: " + std::to_string(start));
            size_t end = BufferStr.find("\r\n", start);
            //LOG_DEBUG_HTTP("index of end: " + std::to_string(end));
            std::string Length = BufferStr.substr(start+16, end - start + 1);
            //LOG_DEBUG_HTTP("Content-Length: " + Length);
            //LOG_DEBUG_HTTP("Received-Length: " + std::to_string((BufferStr.substr(BufferStr.find("\r\n\r\n"))).length() - 4));
            if (std::stoi(Length) == (BufferStr.substr(BufferStr.find("\r\n\r\n"))).length() - 4) ConnectedHTTP = false; break;
        }
        else if (((BufferStr.find("\r\n\r\n") != std::string::npos) && (BufferStr.find("GET") != std::string::npos))) {
            ConnectedHTTP = false; 
            break;
        }
    }
}

// Function for generating response, second and last parameter are optional, second parameter will be "text/plain" if not specified
std::string ResponseGen(std::string HTTPCode, std::string ContentType = "text/plain", std::string Message = "") {
    std::string result;
    if ((HTTPCode.find("204") != std::string::npos) || Message.empty()) {
        result =
            "HTTP/1.1 204 No Content\n"\
            "Access-Control-Allow-Origin: *\r\n" \
            "Server: " + STModInfo.id + "/" + STModInfo.version + "\r\n\r\n";
        return result;
    }
    result =
            "HTTP/1.1 " + HTTPCode + "\r\n" \
            "Content-Length: " + std::to_string(Message.length()) + "\r\n" \
            "Content-Type: " + ContentType + "\r\n" \
            "Access-Control-Allow-Origin: *\r\n" \
            "Server: " + STModInfo.id + "/" + STModInfo.version + "\r\n\r\n" + \
            Message;
    return result;
}

void STManager::HandleRequestHTTP(int client_sock) {
    unsigned int length = 4096+1;
    char* buffer = 0;
    std::string response;
    std::string messageStr;
    buffer = new char[length];
    ReadXBytes(client_sock, length, buffer);
    std::string BufferStr(buffer);
    delete[] buffer;
    #define ROUTE_START()       if ((BufferStr.find("GET / ") != std::string::npos) || (BufferStr.find("GET /index") != std::string::npos))
    #define ROUTE(METHOD,URI)    else if ((BufferStr.find(METHOD " " URI " ") != std::string::npos) ||  (BufferStr.find(METHOD " " URI "/ ") != std::string::npos))
    #define ROUTE_GET(URI)      ROUTE("GET", URI)
    #define ROUTE_POST(URI)      ROUTE("POST", URI)
    #define ROUTE_PUT(URI)      ROUTE("PUT", URI)
    ROUTE_START() {
        messageStr = constructResponse();
        response = ResponseGen("200 OK", "application/json", messageStr);
    }
    ROUTE_GET("/cover/base64") {
        COVER:
        std::string stats = "data:image/jpg;base64," + STManager::coverImageBase64;
        if (stats.empty()) goto NotFound;
        response = ResponseGen("200 OK", "text/plain", stats);
    }
    ROUTE_GET("/cover.jpg") {
        std::string stats = STManager::coverImageJPG;
        if (stats.empty()) goto NotFound;
        response = ResponseGen("200 OK", "image/jpg", stats);
    }
    ROUTE_GET("/config") {
        CONFIG:
        std::string stats = constructConfigResponse();
        if (stats.empty()) goto NotFound;
        response = ResponseGen("200 OK", "application/json", stats);
    }
    ROUTE_PUT("/config") {
        PCONFIG:
        size_t start = BufferStr.find("{");
        LOG_DEBUG_HTTP("index of {: " + std::to_string(start));
        size_t end = BufferStr.find("}");
        LOG_DEBUG_HTTP("index of }: " + std::to_string(end));
        std::string json = BufferStr.substr(start,  end - start + 1);
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

        //response =  "HTTP/1.1 204 No Content\n"\
        //            "Access-Control-Allow-Origin: *\r\n" \
        //            "Server: " + STModInfo.id + "/" + STModInfo.version + "\r\n\r\n";
        response = ResponseGen("204");
    }
    ROUTE_GET("/teapot") {
        response = ResponseGen("418 I'm a teapot", "text/html", 
                        "<!DOCTYPE html> "\
                        "<html> "\
                        "<head> <title>streamer-tools - 418 I'm a teapot</title> "\
                        "<link href='https://fonts.googleapis.com/css?family=Open+Sans:400,400italic,700,700italic' rel='stylesheet' type='text/css'> "\
                        "<style> body { color: #EEEEEE; background-color: #202225; font-size: 14px; font-family: 'Open Sans'; } </style> "\
                        "</head> "\
                        "<body> <div style=\"font-size: 30px;\">Streamer-tools - 418 I'm a teapot</div> "\
                        "<div style=\"color: #888; margin-bottom: 10px; padding-left: 20px;\">The requested entity body is short and stout.</div> "\
                        "<div style=\"font-size: 18px; margin-top: 30px; border-top: solid #BBBBBB 2px; padding: 10px; width: fit-content;\"><i>" + STModInfo.id + "/" + STModInfo.version + " (" + headsetType + ") server at " + STManager::localIP + ":" + std::to_string(PORT_HTTP) + "</i></div> "\
                        "</body> </html>"); // Yes this is long but page is pretty-ish
    }
    else {
        // 404 or invalid
        NotFound:
        response = ResponseGen("404 Not Found", "text/html", 
                        "<!DOCTYPE html> "\
                        "<html> "\
                        "<head> <title>streamer-tools - 404 Not found</title> "\
                        "<link href='https://fonts.googleapis.com/css?family=Open+Sans:400,400italic,700,700italic' rel='stylesheet' type='text/css'> "\
                        "<style> body { color: #EEEEEE; background-color: #202225; font-size: 14px; font-family: 'Open Sans'; } </style> "\
                        "</head> "\
                        "<body> <div style=\"font-size: 30px;\">Streamer-tools - 404 Not found</div> "\
                        "<div style=\"color: #888; margin-bottom: 10px; padding-left: 20px;\">The endpoint you were looking for could not be found.</div> "\
                        "<div style=\"font-size: 18px; margin-top: 30px; border-top: solid #BBBBBB 2px; padding: 10px; width: fit-content;\"><i>" + STModInfo.id + "/" + STModInfo.version + " ("+ headsetType +") server at " + STManager::localIP + ":" + std::to_string(PORT_HTTP) + "</i></div> "\
                        "</body> </html>"); // Yes this is long but page is pretty-ish
    }
    if (!response.empty()) {
        SendResponse: // Just incase we ever need to use goto SendResponse to skip over code
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