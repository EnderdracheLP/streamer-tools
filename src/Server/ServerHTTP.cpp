#define RAPIDJSON_HAS_STDSTRING 1 // Enable rapidjson's support for std::string
#define NO_CODEGEN_USE
#include "ServerHeaders.hpp"
#include "STmanager.hpp"
#include "Config.hpp"

#include "UnityEngine/ImageConversion.hpp"
#include "UnityEngine/Texture2D.hpp"
#include "Unity/Collections/NativeArray_1.hpp"
#include "System/Convert.hpp"

//LoggerContextObject HTTPLogger;

std::string lastClientIP;

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
        // For normal builds we only want to log Client Connected if the IP is different from the Client that connected on the last Request
#ifdef DEBUG_BUILD
#if HTTP_LOGGING == 1
        logger.info("HTTP: Client connected with address: %s", ClientIP.c_str());
#endif
#else  
        if (lastClientIP != ClientIP) logger.info("HTTP: Client connected with address: %s", ClientIP.c_str());
        lastClientIP = ClientIP;
#endif
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
void STManager::ReadRequest(int socket, unsigned int x, char* buffer)
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
// WARNING: Unfinished function not working, can be used for file downloads
// Function for generating response, fourth parameter is optional, fourth parameter will be "multipart/mixed" if not specified
//bool STManager::MultipartResponseGen(int client_sock, std::string TypeOfMessage, std::string HTTPCode, std::string ContentType = "multipart/mixed") {
//    if (ContentType.find("multipart/") == std::string::npos) {
//        getLogger().error("ContentType not known"); return false;
//    }
//        std::string result;
//        result =
//        "HTTP/1.1 " + HTTPCode + "\r\n" \
//        "Content-Type: " + ContentType + "; boundary=CoolBoundaryName\r\n" \
//        "Access-Control-Allow-Origin: *\r\n" \
//        "Server: " + STModInfo.id + "/" + STModInfo.version + "\r\n\r\n" \
//        "--CoolBoundaryName";
//
//    if (write(client_sock, result.c_str(), result.length()) == -1) { // Then send the string
//        getLogger().error("HTTP: Error sending Response: \n%s", strerror(errno));
//        return false;
//    }
//    LOG_DEBUG_HTTP("HTTP Response: \n%s", result.c_str());
//    bool ClientConnected = true;
//    std::string Message;
//    while (ClientConnected) {
//        if (TypeOfMessage == "StandardResponse") Message = STManager::constructResponse();
//        result =
//            "Content-Length: " + std::to_string(Message.length()) + "\r\n" \
//            "Content-Type: application/json\r\n\r\n" + \
//            Message + "\r\n" + \
//            "--CoolBoundaryName";
//        SendResponse: // Just incase we ever need to use goto SendResponse to skip over code
//            if (write(client_sock, result.c_str(), result.length()) == -1) { // Then send the string
//                getLogger().error("HTTP: Error sending Response: \n%s", strerror(errno));
//                ClientConnected = false;
//                return false;
//            }
//            LOG_DEBUG_HTTP("HTTP Response: \n%s", result.c_str());
//    }
//    return true;
//}

// Function for generating response, second and last parameter is optional, second parameter will be "text/plain" if not specified
std::string ResponseGen(std::string HTTPCode, std::string ContentType = "text/plain", std::string Message = "", std::string AllowedMethods = "") {
    std::string result;
    if ((HTTPCode.find("204") != std::string::npos) || Message.empty()) {
        result =
            "HTTP/1.1 204 No Content\r\n"\
            "Server: " + STModInfo.id + "/" + STModInfo.version + "\r\n" \
            "Access-Control-Allow-Origin: *\r\n\r\n";
        return result;
    }
    if (!AllowedMethods.empty()) {
        result =
            "HTTP/1.1 " + HTTPCode + "\r\n" \
            "Server: " + STModInfo.id + "/" + STModInfo.version + "\r\n" \
            "Content-Length: " + std::to_string(Message.length()) + "\r\n" \
            "Content-Type: " + ContentType + "\r\n" \
            "Allow: " + AllowedMethods + "\r\n" \
            "Accept-Patch: " + ContentType + "\r\n" \
            "Access-Control-Allow-Origin: *\r\n\r\n" + \
            Message;
        return result;
    }
    else {
        result =
            "HTTP/1.1 " + HTTPCode + "\r\n" \
            "Server: " + STModInfo.id + "/" + STModInfo.version + "\r\n" \
            "Content-Length: " + std::to_string(Message.length()) + "\r\n" \
            "Content-Type: " + ContentType + "\r\n" \
            "Access-Control-Allow-Origin: *\r\n\r\n" + \
            Message;
        return result;
    }
}


std::string STManager::GetCoverImage(std::string ImageFormat = "jpg", bool Base64 = true) {
    std::string result;
    ArrayW<uint8_t> RawCoverbytesArray;
    LOG_DEBUG_HTTP("Ptr CoverTexture is: %p", coverTexture);
    if (ImageFormat == "jpg") {
        RawCoverbytesArray = UnityEngine::ImageConversion::EncodeToJPG(coverTexture);
        LOG_DEBUG_HTTP("Ptr RawCoverbytesArray is: %p", RawCoverbytesArray);
    }
    else if (ImageFormat == "png") {
        RawCoverbytesArray = UnityEngine::ImageConversion::EncodeToPNG(coverTexture);
        LOG_DEBUG_HTTP("Ptr RawCoverbytesArray is: %p", RawCoverbytesArray);
    }
    if (!RawCoverbytesArray) return result = "";
    if (Base64) {
        return result = "data:image/" + ImageFormat + ";base64," + std::string(System::Convert::ToBase64String(RawCoverbytesArray));
    }
    else {
        std::string data(reinterpret_cast<char*>(RawCoverbytesArray.begin()), RawCoverbytesArray.Length());
        return result = data;
    }
    return result = "";
}

void STManager::SendResponseHTTP(int client_sock, std::string response) {
    if (!response.empty()) {
    SendResponse: // Just incase we ever need to use goto SendResponse to skip over code
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

bool STManager::HandleConfigChange(int client_sock, std::string BufferStr) {
    size_t start = BufferStr.find("{");
    LOG_DEBUG_HTTP("index of {: %s", std::to_string(start).c_str());
    size_t end = BufferStr.find("}");
    LOG_DEBUG_HTTP("index of }: %s", std::to_string(end).c_str());
    std::string json = BufferStr.substr(start, end - start + 1);
    LOG_DEBUG_HTTP("json: %s", json.c_str());
    rapidjson::Document document;
    if (document.Parse(json).HasParseError()) {
        SendResponseHTTP(client_sock, ResponseGen("400 Bad Request", "text/html",
            "<!DOCTYPE html> "\
            "<html> "\
            "<head> <title>streamer-tools - 400 Bad Request</title> "\
            "<link href='https://fonts.googleapis.com/css?family=Open+Sans:400,400italic,700,700italic' rel='stylesheet' type='text/css'> "\
            "<style> body { color: #EEEEEE; background-color: #202225; font-size: 14px; font-family: 'Open Sans'; } </style> "\
            "</head> "\
            "<body> <div style=\"font-size: 30px;\">Streamer-tools - 400 Bad Request</div> "\
            "<div style=\"color: #888; margin-bottom: 10px; padding-left: 20px;\">The request the Server received was invalid.</div> "\
            "<div style=\"font-size: 18px; margin-top: 30px; border-top: solid #BBBBBB 2px; padding: 10px; width: fit-content;\"><i>" + STModInfo.id + "/" + STModInfo.version + " (" + headsetType + ") server at " + STManager::localIP + ":" + std::to_string(PORT_HTTP) + "</i></div> "\
            "</body> </html>")); // Yes this is long but page is pretty-ish));
        return true;
    }
    else {
        // TODO: Maybe use this in the future for Parsing
        //static const char* kTypeNames[] =
        //{ "Null", "False", "True", "Object", "Array", "String", "Number" };

        //for (auto& m : document.GetObject()) {
        //    logger.debug("Type of member %s is %s\n",
        //        m.name.GetString(), kTypeNames[m.value.GetType()]);
        //    if (document.FindMember)
        //}
        if (document.FindMember("lastChanged") != document.MemberEnd() && document["lastChanged"].IsInt() && document["lastChanged"].GetInt() > getModConfig().LastChanged.GetValue()) {
            bool didValueChange = false;
            if (document.FindMember("decimals") != document.MemberEnd() && document["decimals"].IsInt()) {
                getModConfig().DecimalsForNumbers.SetValue(document["decimals"].GetInt());
                didValueChange = true;
            }
            static const char* BoolModConfigValueNames[] = 
            { "dontenergy", "dontmpcode", "alwaysmpcode", "alwaysupdate" };
            ConfigUtils::ConfigValue<bool>* CUBoolModConfigValueNames[] =
            { &getModConfig().DontEnergy, &getModConfig().DontMpCode, &getModConfig().AlwaysMpCode, &getModConfig().AlwaysUpdate };
            for (int i = 0; i < sizeof(BoolModConfigValueNames)/sizeof(char*); ++i) {
                if (document.FindMember(BoolModConfigValueNames[i]) != document.MemberEnd() && document[BoolModConfigValueNames[i]].IsBool()) {
                    CUBoolModConfigValueNames[i]->SetValue(document[BoolModConfigValueNames[i]].GetBool());
                    didValueChange = true;
                }
            }
            if (didValueChange) {
                getModConfig().LastChanged.SetValue(document["lastChanged"].GetInt());
                MakeConfigUI(true);

                SendResponseHTTP(client_sock, ResponseGen("200 OK", "application/json", constructConfigResponse(), "GET, PATCH"));
                return true;
            }
            else return false;
        }
        else {
            SendResponseHTTP(client_sock, ResponseGen("422 Unprocessable Entity", "text/html",
                "<!DOCTYPE html> "\
                "<html> "\
                "<head> <title>streamer-tools - 422 Unprocessable Entity</title> "\
                "<link href='https://fonts.googleapis.com/css?family=Open+Sans:400,400italic,700,700italic' rel='stylesheet' type='text/css'> "\
                "<style> body { color: #EEEEEE; background-color: #202225; font-size: 14px; font-family: 'Open Sans'; } </style> "\
                "</head> "\
                "<body> <div style=\"font-size: 30px;\">Streamer-tools - 422 Unprocessable Entity</div> "\
                "<div style=\"color: #888; margin-bottom: 10px; padding-left: 20px;\">The request the Server received contained an entity that could not be processed.</div> "\
                "<div style=\"font-size: 18px; margin-top: 30px; border-top: solid #BBBBBB 2px; padding: 10px; width: fit-content;\"><i>" + STModInfo.id + "/" + STModInfo.version + " (" + headsetType + ") server at " + STManager::localIP + ":" + std::to_string(PORT_HTTP) + "</i></div> "\
                "</body> </html>")); // Yes this is long but page is pretty-ish));
            return true;
        }
        return false;
    }
    static const char* kTypeNames[] =
    { "Null", "False", "True", "Object", "Array", "String", "Number" };

    for (auto& m : document.GetObject()) {
    logger.debug("Type of member %s is %s\n",
        m.name.GetString(), kTypeNames[m.value.GetType()]);
    }
    return false;
}

#define ROUTE_START()       if ((BufferStr.find("GET / ") != std::string::npos) || (BufferStr.find("GET /index") != std::string::npos))
#define ROUTE(METHOD,URI)    if ((BufferStr.find(METHOD " " URI " ") != std::string::npos) ||  (BufferStr.find(METHOD " " URI "/ ") != std::string::npos) ||  (BufferStr.find(METHOD " " URI "?") != std::string::npos))
#define ROUTE_PATH(PATH)    else if (BufferStr.find(PATH" ") == std::string::npos && BufferStr.find(PATH) != std::string::npos)
#define ROUTE_GET(URI)      ROUTE("GET", URI)
#define ROUTE_POST(URI)      ROUTE("POST", URI)
#define ROUTE_PUT(URI)      ROUTE("PUT", URI)
#define ROUTE_PATCH(URI)      ROUTE("PATCH", URI)


void STManager::HandleRequestHTTP(int client_sock) {
    unsigned int length = 4096+1;
    char* buffer = 0;
    std::string response;
    std::string messageStr;
    buffer = new char[length];
    ReadRequest(client_sock, length, buffer);
    std::string BufferStr(buffer);
    delete[] buffer;
    ROUTE_START() {
        ROUTE_GET("/index.json") goto NormalResponse;
        //if (BufferStr);
            response = ResponseGen("200 OK", "text/html", "<!DOCTYPE html>\n<html>\n    <head>\n        <title>streamer-tools</title>\n        <link href=\'https://fonts.googleapis.com/css?family=Open+Sans:400,400italic,700,700italic\' rel=\'stylesheet\' type=\'text/css\'>\n        <style>\n            body {\n                color: #EEEEEE;\n                background-color: #202225;\n                font-size: 14px;\n                font-family: \'Open Sans\';\n            }\n            a {\n                color: #30A2EC;\n            }\n        </style>\n    </head>\n    <body>\n        <div style=\"font-size: 30px;\">Streamer-tools</div>\n        <div style=\"color: #EEEEEE; margin-bottom: 10px; padding-left: 20px;\">\n            Enhance your Beat Saber stream with overlays and more. This mod provides all data necessary for the client on your PC to work.\n            <br/>\n            Get <a href=\"https://github.com/ComputerElite/streamer-tools-client/\">the client by ComputerElite</a> or <a href=\"overlays.html\">Open the overlays in your browser</a> (not every overlay may work) to start your Beat Saber streamer life\n        </div>\n        <div style=\"font-size: 18px; margin-top: 30px; border-top: solid #BBBBBB 2px; padding: 10px; width: fit-content;\"><i>" + STModInfo.id + "/" + STModInfo.version + " (" + headsetType + ") server at " + STManager::localIP + ":" + std::to_string(PORT_HTTP) + "</i></div>\n    </body>\n</html>");
            SendResponseHTTP(client_sock, response);
            return;
    }
    ROUTE_GET("/data") {
    NormalResponse:
        messageStr = constructResponse();
        response = ResponseGen("200 OK", "application/json", messageStr);
        SendResponseHTTP(client_sock, response);
        return;
    }
    ROUTE_PATH("/cover/") {
        if (CoverStatus == Running) {
            std::unique_lock<std::mutex> lk(STManager::CoverLock);
            STManager::cv.wait_for(lk, std::chrono::seconds(2));
        }
        std::string tempStr;
        ROUTE_GET("/cover/base64") {
        COVER_B64_JPG:
            if (!coverTexture) goto NotFound;
            else if (CoverChanged[0]) {
                LOG_DEBUG_HTTP("%d", CoverChanged[0]);
                tempStr = GetCoverImage();
                if (!tempStr.empty()) coverImageBase64 = tempStr;
                CoverChanged[0] = false;
            }
            else LOG_DEBUG_HTTP("CoverImageUnchanged");

            response = ResponseGen("200 OK", "text/plain", coverImageBase64);
            SendResponseHTTP(client_sock, response);
            return;
        }
        ROUTE_GET("/cover/base64/png") {
        COVER_B64_PNG:
            if (!coverTexture) goto NotFound;
            else if (CoverChanged[1]) {
                tempStr = GetCoverImage("png");
                if (!tempStr.empty()) coverImageBase64PNG = tempStr;
                CoverChanged[1] = false;
            }
            else LOG_DEBUG_HTTP("CoverImageUnchanged");

            response = ResponseGen("200 OK", "text/plain", coverImageBase64PNG);
            SendResponseHTTP(client_sock, response);
            return;
        }
        ROUTE_GET("/cover/cover.jpg") {
            if (!coverTexture) goto NotFound;
            else if (CoverChanged[2]) {

                tempStr = GetCoverImage("jpg", false);
                if (!tempStr.empty()) coverImageJPG = tempStr;
                CoverChanged[2] = false;
            }
            else LOG_DEBUG_HTTP("CoverImageUnchanged");

            response = ResponseGen("200 OK", "image/jpeg", /*stats*/ coverImageJPG);
            SendResponseHTTP(client_sock, response);
            return;
        }
        ROUTE_GET("/cover/cover.png") {
            if (!coverTexture) goto NotFound;       
            else if (CoverChanged[3]) {
                tempStr = GetCoverImage("png", false);
                if (!tempStr.empty()) coverImagePNG = tempStr;
                CoverChanged[3] = false;
            }
            else LOG_DEBUG_HTTP("CoverImageUnchanged");

            response = ResponseGen("200 OK", "image/png", /*stats*/ coverImagePNG);
            SendResponseHTTP(client_sock, response);
            return;
        }
    }
    ROUTE_GET("/config") {
    CONFIG:
        configFetched = true;
        response = ResponseGen("200 OK", "application/json", constructConfigResponse(), "GET, PATCH");
        SendResponseHTTP(client_sock, response);
        return;
    }
    ROUTE_PATCH("/config") {
    PCONFIG:
        if (!HandleConfigChange(client_sock, BufferStr)) SendResponseHTTP(client_sock, ResponseGen("204"));
        return;
    }
    ROUTE_GET("/positions") {
        if (STManager::Head && STManager::VR_Right && STManager::VR_Left && !(location == 0 || location >= 4 )) {
            SendResponseHTTP(client_sock, ResponseGen("200 OK", "application/json", constructPositionResponse()));
            return;
        }
        else {
            SendResponseHTTP(client_sock, ResponseGen("204"));
            return;
        }
    }
    ROUTE_GET("/info") {
    ModInfoResponse:
        messageStr = multicastResponse();
        response = ResponseGen("200 OK", "application/json", messageStr);
        SendResponseHTTP(client_sock, response);
        return;
    }
    ROUTE_GET("/loader.html") {
        response = ResponseGen("200 OK", "text/html", "<!DOCTYPE html>\n<html>\n    <head>\n        <title>test</title>\n    </head>\n    <body>\n        Yeah so it load Something\n        <script>\n            var url = new URL(window.location.href);\n            var uri = url.searchParams.get(\"uri\");\n            var html = MakeTextGetRequest(uri)\n            var baseDir = uri\n            if(baseDir.endsWith(\"/\")) baseDir = baseDir.substring(0, baseDir.length - 1)\n            baseDir = baseDir.substring(0, baseDir.lastIndexOf(\"/\"))\n            //Check if base dir exceeds website domain\n            const websiteRegex = /^https?\\:\\/\\/([^/]+(\\/)?)$/g\n            if(uri.match(websiteRegex)) {\n                baseDir = uri.endsWith(\"/\") ? uri : uri + \"/\"\n            }\n            // Get every match for a relative uri and replace it with an absolute one\n            const reg = /(\"|`|\')(((\\.\\.\\/)+)|\\/)?[a-zA-Z0-9\\-_\\.\\/\\{\\}\\$]+(\"|`|\')/g\n            var links = []\n            while ((match = reg.exec(html)) !== null) {\n                match[0] = match[0].substring(1, match[0].length - 1)\n                if(!match[0].includes(\".\") && !match[0].endsWith(\"/\") && !match[0].startsWith(\"//\") || match[0].startsWith(\"///\") || match[0].startsWith(\"$\")) continue;\n                console.log(`Found ${match[0]} start=${match.index} end=${reg.lastIndex}.`);\n                var replacement = GetAbsoluteLink(baseDir, match[0])\n                links.push({\n                    absolute: replacement,\n                    relative: match[0],\n                    start: match.index,\n                    end: reg.lastIndex\n                })\n                console.log(\"replacing with \" + replacement)\n            }\n            //Start replacement\n            var length = 0\n            links.forEach(link => {\n                html = html.substring(0, link.start + 1 + length) + html.substring(link.end - 1 + length, html.length)\n                html = InsertString(link.absolute, link.start + 1 + length, html)\n                length += link.absolute.length - link.relative.length\n            })\n            \n            const scriptReg = /<script( src=\".+?\")?((\\/>)|(>.*?<\\/script>))/gs\n            const headStart = html.indexOf(\"<head>\") + 6\n            var scripts = []\n            while ((match = scriptReg.exec(html)) !== null) {\n                scripts.push(match[0]);\n                console.log(\"moved script \" + match[0] + \" into head\")\n            }\n            scripts.forEach(s => {\n                html = html.replace(s, \"\")\n            })\n            \n            document.documentElement.innerHTML = html\n            const srcRegex = /src=\".+?\"/gs\n            var i = 0\n            scripts.forEach(e => {\n                console.log(i)\n                i++\n                var script = document.createElement(\"script\")\n                if(e.substring(e.indexOf(\"<\"), e.indexOf(\">\")).match(srcRegex))\n                {\n                    var found = srcRegex.exec(e.substring(0, e.indexOf(\">\")))[0];\n                    script.src = found.substring(5, found.length - 1)\n                } else script.appendChild(document.createTextNode(e.substring(e.indexOf(\">\") + 1, e.lastIndexOf(\"<\") - 1)))\n                document.head.appendChild(script)\n            })\n            function GetAbsoluteLink(baseUri, relativeUri) {\n                var absolute = baseUri;\n                if(relativeUri.startsWith(\"../\")) {\n                    var tmp = absolute.split(\"/\")\n                    tmp.pop();\n                    absolute = tmp.join(\"/\")\n                    absolute = GetAbsoluteLink(absolute, relativeUri.substring(3, relativeUri.length))\n                } else if(relativeUri.startsWith(\"//\")) {\n                    absolute = baseUri.substring(0, baseUri.indexOf(\"/\")) + relativeUri\n                } else {\n                    absolute += \"/\" + relativeUri\n                }\n                return absolute\n            }\n            function InsertString(toInsert, position, text) {\n                return [text.slice(0, position), toInsert, text.slice(position)].join(\'\')\n            }\n            function MakeTextGetRequest(url) {\n                var request = new XMLHttpRequest();\n                request.open(\'GET\', url, false);\n                request.send(null);\n                if (request.status == 200) {\n                    return request.responseText;\n                } else {\n                    return \"Something went wrong: \" + request.status;\n                }\n            }\n        </script>\n    </body>\n</html>");
        SendResponseHTTP(client_sock, response);
        return;
    }
    ROUTE_GET("/snake") {
        response = ResponseGen("200 OK", "text/html", "<!DOCTYPE html>\n<html>\n    <head>\n    </head>\n    <body>\n        <script>\n            window.location.href = \"/loader.html?uri=https://computerelite.github.io/fun/snake.html\"\n        </script>\n    </body>\n</html>");
        SendResponseHTTP(client_sock, response);
        return;
    }
    ROUTE_GET("/overlays.html") {
        response = ResponseGen("200 OK", "text/html", "<!DOCTYPE html>\n<html>\n    <head>\n        <title>streamer-tools</title>\n        <link href=\'https://fonts.googleapis.com/css?family=Open+Sans:400,400italic,700,700italic\' rel=\'stylesheet\' type=\'text/css\'>\n        <style>\n            body {\n                color: #EEEEEE;\n                background-color: #202225;\n                font-size: 14px;\n                font-family: \'Open Sans\';\n            }\n            a {\n                color: #30A2EC;\n            }\n        </style>\n    </head>\n    <body>\n        <div style=\"font-size: 30px;\">Streamer-tools - Overlays</div>\n        <div style=\"color: #EEEEEE; margin-bottom: 10px; padding-left: 20px;\">\n            Here are all currently available overlays (not every overlay may work):\n            <br/>\n            <div id=\"overlays\">\n\n            </div>\n        </div>\n        <div style=\"font-size: 18px; margin-top: 30px; border-top: solid #BBBBBB 2px; padding: 10px; width: fit-content;\"><i>" + STModInfo.id + "/" + STModInfo.version + " ("+ headsetType +") server at " + STManager::localIP + ":" + std::to_string(PORT_HTTP) + "</i></div>\n        <script>\n        const baseUrl = window.location.href.split(\"?\")[0];\n        function FormatToHTML(overlay) {\n            var link = \"loader.html?uri=\" + getLink(overlay) + \"&ip=" + STManager::localIP + "\"\n            return `<div style=\"margin-top: 20px;\">${overlay.Name}:<br/><div style=\"font-size: 14px;\">URL: <a style=\"font-size: 14px;\" target=\"_blank\" href=\"${link}\">${link}</a><br/><input type=\'button\' onclick=\'Copy(\"${link}\")\' style=\"margin-bottom: 5px;\" value=\"Copy URL\"><br/></div></div>`\n        }\n\n        function getLink(overlay) {\n            for(let i = 0; i < overlay.downloads.length; i++) {\n                if(overlay.downloads[i].IsEntryPoint) return overlay.downloads[i].URL\n            }\n        }\n\n        fetch(`https://computerelite.github.io/tools/Streamer_Tools_Quest_Overlay/overlays.json`).then(res => {\n            res.json().then(json => {\n                document.getElementById(\"overlays\").innerHTML = \"\";\n                json.overlays.forEach(overlay => {\n                    document.getElementById(\"overlays\").innerHTML += FormatToHTML(overlay);\n                })\n            })\n        })\n        </script>\n    </body>\n</html>");
        SendResponseHTTP(client_sock, response);
        return;
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
        SendResponseHTTP(client_sock, response);
        return;
    }
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
    SendResponseHTTP(client_sock, response);
}
