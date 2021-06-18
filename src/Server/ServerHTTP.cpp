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


std::string STManager::GetCoverImage(std::string ImageFormat = "jpg", bool Base64 = true) {
    std::string result;
    Array<uint8_t>* RawCoverbytesArray;
    if (ImageFormat == "jpg") {
        RawCoverbytesArray = UnityEngine::ImageConversion::EncodeToJPG(coverTexture);
    }
    else if (ImageFormat == "png") {
        RawCoverbytesArray = UnityEngine::ImageConversion::EncodeToPNG(coverTexture);
    }
    if (Base64) {
            return result = "data:image/" + ImageFormat + ";base64," + to_utf8(csstrtostr(System::Convert::ToBase64String(RawCoverbytesArray)));
    }
    else {
        std::string data(reinterpret_cast<char*>(RawCoverbytesArray->values), RawCoverbytesArray->Length());
        return result = data;
    }
    return result = "";
}

void STManager::HandleRequestHTTP(int client_sock) {
    unsigned int length = 4096+1;
    char* buffer = 0;
    std::string response;
    std::string messageStr;
    buffer = new char[length];
    ReadRequest(client_sock, length, buffer);
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
    // This does not yet work
    //ROUTE_GET("/LiveData") {
    //    bool LiveData = MultipartResponseGen(client_sock, "StandardResponse", "200 OK", "multipart/x-mixed-replace");
    //    //if (!LiveData) close(client_sock); ConnectedHTTP = false; goto NotFound;
    //        ConnectedHTTP = false;
    //        close(client_sock);
    //        return;
    //}
    ROUTE_GET("/cover/base64") {
    COVER_B64_JPG:
        if (!coverTexture) goto NotFound;
        else if (CoverChanged[0]) {
            coverImageBase64 = GetCoverImage();
            CoverChanged[0] = false;
        }
        else LOG_DEBUG_HTTP("CoverImageUnchanged");

        response = ResponseGen("200 OK", "text/plain", coverImageBase64);
    }
    ROUTE_GET("/cover/base64/png") {
    COVER_B64_PNG:
        if (!coverTexture) goto NotFound;
        else if (CoverChanged[1]) {
            coverImageBase64PNG = GetCoverImage("png");
            CoverChanged[1] = false;
        }
        else LOG_DEBUG_HTTP("CoverImageUnchanged");

        response = ResponseGen("200 OK", "text/plain", coverImageBase64PNG);
    }
    ROUTE_GET("/cover.jpg") {
        if (!coverTexture) goto NotFound;
        else if (CoverChanged[2]) {
            coverImageJPG = GetCoverImage("jpg", false);
            CoverChanged[2] = false;
        }
        else LOG_DEBUG_HTTP("CoverImageUnchanged");

        response = ResponseGen("200 OK", "image/jpg", /*stats*/ coverImageJPG);
    }
    ROUTE_GET("/cover.png") {
        if (!coverTexture) goto NotFound;       
        else if (CoverChanged[3]) {
            coverImagePNG = GetCoverImage("png", false);
            CoverChanged[3] = false;
        }
        else LOG_DEBUG_HTTP("CoverImageUnchanged");

        response = ResponseGen("200 OK", "image/png", /*stats*/ coverImagePNG);
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
        
        MakeConfigUI(true);

        response = ResponseGen("204");
    }
    ROUTE_GET("/overlay") {
        response = ResponseGen("200 OK", "text/html", "<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.0 Strict//EN\" \"http://www.w3.org/TR/xhtml1/DTD/xhtml1-strict.dtd\"> <html xmlns=\"http://www.w3.org/1999/xhtml\" lang=\"en\">\n<head>\n<meta charset=\"utf-8\"> <meta name=\"viewport\" content=\"width=device-width\">\n<title>Streamer Tools Viewer Quest Overlays</title>\n</head>\n<style>\nhtml{background:#1c1c1c}body{background:#262626;font-family:arial,sans-serif;color:LightGray;margin:5% auto;max-width:1200px;min-width:min-content;-webkit-box-shadow:0 1px 3px rgba(0,0,0,.13);box-shadow:0 1px 3px rgba(0,0,0,.13)}h1{border-bottom:1px solid #fe2d2d;clear:both;color:White;font-size:24px;margin:30px 0 0;padding:0;padding-bottom:7px}#SelectOverlayPage{position:absolute;left:3%;right:3%;min-width:min-content}@media screen and (max-width:400px){.SelectOverlayDiv{padding-bottom:0}}#SelectOverlayPage p,SelectOverlayPage .SelectOverlayDiv{font-size:14px;line-height:1.5;margin:25px 0 20px}ul li{margin-bottom:10px}a:link{color:DodgerBlue}a:hover{color:DeepSkyBlue}a:visited{color:RoyalBlue}a:active{color:SteelBlue}img{border:none}img.logoimg{margin-top:20px;max-width:100%}\n</style>\n<body id=\"SelectOverlayPage\">\n<div class=\"SelectOverlayDiv\">\n<blockquote>\n<h2><center>Streamer-Tools - Overlays</center></h2>\n<p>Select the Overlay you want:\n<ul>\n<li><b><a href=\"overlay/orig\">Original</a></b> - The Original Overlay by ComputerElite.\n<li><b><a href=\"overlay/reselim\">Reselim</a></b> - Overlay by Reselim</li>\n<li><b><a href=\"overlay/bandoot\">Bandoot</a></b> - The Bandoot Overlay.\n</ul>\n<div style=\"font-size: 18px; margin-top: 30px; border-top: solid #BBBBBB 2px; padding: 10px; width: fit-content;\"><i>" + STModInfo.id + "/" + STModInfo.version + " (" + headsetType + ") server at " + STManager::localIP + ":" + std::to_string(PORT_HTTP) + "</i></div>\n</blockquote>\n</div>\n</body> </html> </body> </html>\n");
    }
    ROUTE_GET("/overlay/pulling.js") {
        std::string pulling_js = "function format(text, returnType = 0) {\n    return text == \"\" ? (returnType >= 1 ? (returnType >= 2 ? \"SS\" : \"0\") : \"N/A\") : text\n}\n\nfunction intToDiff(diff) {\n    switch (diff)\n    {\n        case 0:\n            return \"Easy\";\n        case 1:\n            return \"Normal\";\n        case 2:\n            return \"Hard\";\n        case 3:\n            return \"Expert\";\n        case 4:\n            return \"Expert +\";\n    }\n    return \"Unknown\";\n}\n\nfunction SetPercentage(percentage) {\n    try {\n        bar.style.width = (percentage * 100) + \"%\"\n    } catch {}\n}\n\n\nvar lastID = \"\";\nvar lastSongKey = \"\";\nvar got404 = false;\n\nfunction SetImage(id) {\n    if(id.startsWith(\"custom_level_\") && id != lastID) {\n        fetch(\"https://beatsaver.com/api/maps/by-hash/\" + id.replace(\"custom_level_\", \"\")).then((result) => {\n            result.json().then((json) => {\n                try {\n                    key.innerHTML = json[\"key\"]\n                } catch {}\n                \n                try {\n                    if(lastSongKey != \"\") {\n                        try {\n                            prekeyContainer.style.display = \"inline\"\n                        } catch {}\n                    }\n                    prekey.innerHTML = lastSongKey\n                } catch {}\n                lastSongKey = json[\"key\"]\n            })\n        }).catch((err) => {})\n    } else if(useLocalhost && stats[\"fetchedKey\"]) {\n        try {\n            key.innerHTML = stats[\"key\"]\n        } catch {}\n        lastSongKey = stats[\"key\"]\n    }\n    if(id != lastID || got404) {\n        fetch(useLocalhost ? localip + \"cover\" : \"http://\" + ip + \":53502/cover/base64\").then((res) => {\n            res.text().then((base64) => {\n                if(res.status == 404) {\n                    cover.src = \"cover.jpg\"\n                    got404 = true;\n                } else {\n                    cover.src = base64\n                    got404 = false;\n                }\n            })\n        }).catch((err) => {\n            // Fallback to default cover\n            got404 = true\n            cover.src = \"cover.jpg\"\n        })\n    }\n    lastID = id\n}\n\n\nvar bar = document.getElementById(\"energybar\")\nvar barContainer = document.getElementById(\"energybarContainer\")\nvar songName = document.getElementById(\"songName\")\nvar songAuthor = document.getElementById(\"songAuthor\")\nvar mapper = document.getElementById(\"mapper\")\nvar diff = document.getElementById(\"diff\")\nvar combo = document.getElementById(\"combo\")\nvar score = document.getElementById(\"score\")\nvar cover = document.getElementById(\"cover\")\nvar key = document.getElementById(\"key\")\nvar rank = document.getElementById(\"rank\")\nvar percentage = document.getElementById(\"percentage\")\nvar songSub = document.getElementById(\"songSub\")\nvar njs = document.getElementById(\"njs\")\nvar bpm = document.getElementById(\"bpm\")\nvar timePlayed = document.getElementById(\"timePlayed\")\nvar totalTime = document.getElementById(\"totalTime\")\nvar mpCode = document.getElementById(\"mpCode\")\nvar mpCodeContainer = document.getElementById(\"mpCodeContainer\")\nvar prekey = document.getElementById(\"preKey\")\nvar prekeyContainer = document.getElementById(\"preKeyContainer\")\nvar customTextContainer = document.getElementById(\"customText\")\n\nvar williamGayContainer = document.getElementById(\"williamGayContainer\")\nvar pinkCuteContainer = document.getElementById(\"pinkCuteContainer\")\nvar eraCuteContainer = document.getElementById(\"eraCuteContainer\")\n\nvar url_string = window.location.href\nvar url = new URL(url_string);\nvar ip = \"" + STManager::localIP + "\";\nvar rate = url.searchParams.get(\"updaterate\")\nvar decimals = url.searchParams.get(\"decimals\")\n\nvar showmpcode = url.searchParams.get(\"dontshowmpcode\")\nif(showmpcode == null) showmpcode = true;\nelse showmpcode = false\n\nvar alwaysupdate = url.searchParams.get(\"alwaysupdate\")\nif(alwaysupdate == null) alwaysupdate = false;\nelse alwaysupdate = true\n\nvar nosetip = url.searchParams.get(\"nosetip\")\nif(nosetip == null) nosetip = false;\nelse nosetip = true\n\nvar long = url.searchParams.get(\"unnecessarilylongparameterwhichsetsupdateratewithc00lstufftofuqy0u0ffs0youdontwriteitbtwpinkcuteandwilliamgayandblameenderforthisideaandcomputerforimplementingitintotheoverlaysgotabitcarriedawaytypingthissohavefunnowbutwhatifitellyouthisisntdoingwhatyouarethinkingbcicandowhatiwantwithcodeandyouaretypingittogetrickrolledsoicanhavemyfunevenwhenitsnotaprilfirst\")\nif(long != null) {\n    alert(\"You\'re crazy stop typing this stuff. Here have fun:\")\n    window.open(\"https://www.youtube.com/watch?v=dQw4w9WgXcQ\", \"_self\")\n}\n\nvar williamGay = url.searchParams.get(\"williamgay\")\nif(williamGay != null) {\n    try {\n        williamGayContainer.style.display = \"block\"\n    } catch {}\n} else {\n    try {\n        williamGayContainer.style.display = \"none\"\n    } catch {}\n}\n\nvar eraCute = url.searchParams.get(\"eracute\")\nif(eraCute != null) {\n    try {\n        eraCuteContainer.style.display = \"block\"\n    } catch {}\n} else {\n    try {\n        eraCuteContainer.style.display = \"none\"\n    } catch {}\n}\n\nvar pinkCute = url.searchParams.get(\"pinkcute\")\nif(pinkCute != null) {\n    try {\n        pinkCuteContainer.style.display = \"block\"\n    } catch {}\n} else {\n    try {\n        pinkCuteContainer.style.display = \"none\"\n    } catch {}\n}\n\nvar alwaysshowmpcode = url.searchParams.get(\"alwayshowmpcode\")\nif(alwaysshowmpcode == null) alwaysshowmpcode = false;\nelse alwaysshowmpcode = true\n\nvar chartwidth = url.searchParams.get(\"chartwidth\")\nif(chartwidth == null) chartwidth = 100;\n\nvar showenergyBar = url.searchParams.get(\"dontshowenergy\")\nif(showenergyBar == null) showenergyBar = true;\nelse showenergyBar = false\n\nvar customText = url.searchParams.get(\"customtext\");\nif(customText != null && customText != \"\") {\n    try {\n        customTextContainer.innerHTML = customText\n    } catch {}\n} else {\n    try {\n        customTextContainer.style.display = \"none\"\n    } catch {}\n}\n\nif(ip == null || ip == \"\") {\n    ip = prompt(\"Please enter your Quests IP:\", \"192.168.x.x\");\n}\nif(rate == null) rate = 100\nif(decimals == null) decimals = 2\nconsole.log(\"update rate: \" + rate)\nconsole.log(\"decimals for percentage: \" + decimals)\nconsole.log(\"ip: \" + ip)\nconsole.log(\"show mp code: \" + showmpcode)\nconsole.log(\"show energy bar: \" + showenergyBar)\n\nvar useLocalhost = false;\n\ntry {\n    prekeyContainer.style.display = \"none\"\n} catch {}\n\nvar stats = {}\n\nvar firstRequest = true\n\nsetInterval(function() {\n    fetch(useLocalhost ? localip + \"?ip=\" + ip + (nosetip ? \"&nosetip\" : \"\") : \"http://\" + ip + \":53502\").then((response) => {\n        response.json().then((json) => {\n            //console.log(stats)\n            if(json[\"location\"] == 1 || json[\"location\"] == 2 || json[\"location\"] == 3 || json[\"location\"] == 4 || alwaysupdate || firstRequest) {\n                stats = json\n                firstRequest = false\n            } else {\n                stats[\"location\"] = json[\"location\"]\n                stats[\"mpGameId\"] = json[\"mpGameId\"]\n                stats[\"mpGameIdShown\"] = json[\"mpGameIdShown\"]\n            }\n            \n            if(json[\"connected\"] != undefined && !json[\"connected\"]) {\n                basicSetNotConnected()\n            } else {\n                setAll()\n            }\n        })\n    })\n}, rate)\n\nfunction basicSetNotConnected() {\n    try {\n        SetPercentage(1.0)\n    } catch {}\n    try {\n        songName.innerHTML = format(\"Quest disconnected\")\n    } catch {}\n    try {\n        songAuthor.innerHTML = format(\"\")\n    } catch {}\n    try {\n        mapper.innerHTML = format(\"\")\n    } catch {}\n    try {\n        diff.innerHTML = intToDiff(4)\n    } catch {}\n    try {\n        combo.innerHTML = format(0, 1)\n    } catch {}\n    try {\n        score.innerHTML = format(AddComma(0), 1)\n    } catch {}\n    try {\n        rank.innerHTML = format(\"SS\", 2)\n    } catch {}\n    try {\n        percentage.innerHTML = format(trim(100)) + \" %\"\n    } catch {}\n    try {\n        songSub.innerHTML = format(0)\n    } catch {}\n    try {\n        njs.innerHTML = format(trim(0))\n    } catch {}\n    try {\n        bpm.innerHTML = format(trim(0), 1)\n    } catch {}\n    try {\n        mpCode.innerHTML = \"not in lobby\"\n    } catch {}\n    try {\n        mpCodeContainer.style.display = \"none\"\n    } catch {}\n    try {\n        updateTime(10, 5)\n    } catch {}\n}\n\nfunction setAll() {\n    try {\n        SetPercentage(stats[\"energy\"])\n    } catch {}\n    try {\n        songName.innerHTML = format(stats[\"levelName\"])\n    } catch {}\n    try {\n        songAuthor.innerHTML = format(stats[\"songAuthor\"])\n    } catch {}\n    try {\n        mapper.innerHTML = format(stats[\"levelAuthor\"])\n    } catch {}\n    try {\n        diff.innerHTML = intToDiff(stats[\"difficulty\"])\n    } catch {}\n    try {\n        combo.innerHTML = format(stats[\"combo\"], 1)\n    } catch {}\n    try {\n        score.innerHTML = format(AddComma(stats[\"score\"]), 1)\n    } catch {}\n    try {\n        rank.innerHTML = format(stats[\"rank\"], 2)\n    } catch {}\n    try {\n        percentage.innerHTML = format(trim(stats[\"accuracy\"] * 100)) + \" %\"\n    } catch {}\n    try {\n        SetImage(stats[\"id\"])\n    } catch {}\n    try {\n        songSub.innerHTML = format(stats[\"levelSubName\"])\n    } catch {}\n    try {\n        njs.innerHTML = format(trim(stats[\"njs\"]))\n    } catch {}\n    try {\n        bpm.innerHTML = format(trim(stats[\"bpm\"]), 1)\n    } catch {}\n    try {\n        if(stats[\"location\"] == 2 || stats[\"location\"] == 5) {\n            // Is in mp lobby or song\n            if(stats[\"mpGameIdShown\"] && showmpcode || alwaysshowmpcode) {\n                mpCode.innerHTML = format(stats[\"mpGameId\"])\n            } else {\n                mpCode.innerHTML = \"*****\"\n            }\n        } else {\n            mpCode.innerHTML = \"not in lobby\"\n        }\n    } catch {}\n    try {\n        if((stats[\"location\"] == 2 || stats[\"location\"] == 5) && showmpcode) {\n            mpCodeContainer.style.display = \"block\"\n        } else {\n            mpCodeContainer.style.display = \"none\"\n        }\n    } catch {}\n    try {\n        updateTime(stats[\"endTime\"], stats[\"time\"])\n    } catch {}\n\n    try {\n        if(!showenergyBar) {\n            barContainer.style.display = \"none\"\n        }\n    } catch {}\n    try {\n        SetFPS(stats[\"fps\"], chartwidth)\n    } catch {}\n}\n\nfunction AddComma(input) {\n    return input.toString().replace(/\\B(?=(\\d{3})+(?!\\d))/g, \",\");\n}\n\nfunction trim(input) {\n    return input.toFixed(decimals)\n}\n\nfunction ToElapsed(input) {\n    var date = new Date(0);\n    date.setSeconds(input); // specify value for SECONDS here\n    var timeString = date.toISOString().substr(14, 5);\n    return timeString\n}";
        response = ResponseGen("200 OK", "application/javascript", pulling_js);
    }
    ROUTE_GET("/overlay/orig") {
    OverlayOrig:
        std::string Overlay = "<html>\n    <head>\n        <title>Streamer Tools Viewer Quest Overlay</title>\n        <meta property=\"og:site_name\" content=\"ComputerElite\">\n        <meta property=\"og:title\" content=\"Streamer Tools Viewer Quest Overlay\" />\n        <meta property=\"og:description\" content=\"An overlay for Streamer Tools Viewer for Beat Saber on Quest. Original Overlay\" />\n        <meta property=\"og:url\" content=\"https://computerelite.github.io/tools/Streamer_Tools_Quest_Overlay/Overlay1.html\" />\n        <link href=\'https://fonts.googleapis.com/css?family=Open+Sans:400,400italic,700,700italic\' rel=\'stylesheet\' type=\'text/css\'>\n        <link href=\"https://computerelite.github.io/css/standard.css\" type=\"text/css\" rel=\"stylesheet\">\n        <style>\n            body {\n                background-color: #00000000;\n                background-image: none;\n            }\n        </style>\n    </head>\n    <body>\n        <div style=\"position: absolute; left: 10px; top: 10px; background-color: #222222; border-radius: 5px; padding: 10px;\">\n            <div style=\"font-size: 170%;\" id=\"songName\">Song name</div>\n            <div style=\"display: flex;\">\n                <div style=\"flex: 1;\">\n                    <img style=\"width: 100px; height: 100px;\" src=\"https://computerelite.github.io/tools/Streamer_Tools_Quest_Overlay/default.png\" id=\"cover\">\n                </div>\n                <div style=\"flex: 1; padding-left: 10px;\">\n                    <div style=\"font-size: 110%;\">Artist: <div style=\"font-size: 120%;\" id=\"songAuthor\">Artist</div></div>\n                    <div style=\"font-size: 110%; margin-top: 10px; margin-bottom: 10px;\">Mapper: <div style=\"font-size: 120%;\" id=\"mapper\">Mapper</div></div>\n                    <div style=\"font-size: 110%;\" id=\"diff\">diff</div>\n                    <div style=\"display: inline; font-size: 80%;\">Key: <div style=\"display: inline;\" id=\"key\">key</div></div>\n                </div>\n            </div>\n            <div style=\"font-size: 5px; position: absolute; bottom: 0px; right: 0px; background-color: #222222; padding: 2px; border-radius: 2px; \" id=\"pinkCuteContainer\">Pink Cute</div>\n            <div style=\"font-size: 5px; position: absolute; bottom: 0px; right: 50px; background-color: #222222; padding: 2px; border-radius: 2px; \" id=\"eraCuteContainer\">Era Cute</div>\n            <div id=\"mpCodeContainer\">Mp lobby: <div style=\"font-size: 110%; display: inline;\" id=\"mpCode\">*****</div></div>\n            <div>Combo: <div style=\"font-size: 110%; display: inline;\" id=\"combo\">Combo</div></div>\n            <div>Score: <div style=\"font-size: 110%; display: inline;\" id=\"score\">Score</div></div>\n            <div>Rank: <div style=\"font-size: 110%; display: inline;\" id=\"rank\">Rank</div> (<div style=\"font-size: 100%; display: inline;\" id=\"percentage\"></div>)</div>\n            <div style=\"font-size: 110%; margin-top: 5px;\" id=\"customText\"></div>\n        </div>\n        <div id=\"energybarContainer\" style=\"width: 30%; height: 2%; position: absolute; background-color: #222222; border-radius: 3px; margin: auto; bottom: 10px; left: 0px; right: 0px; padding: 1px;z-index: -2;\">\n            <div style=\"font-size: 5px; position: absolute; top: 0px; right: 0px; background-color: #222222; padding: 2px; border-radius: 2px; z-index: -1;\" id=\"williamGayContainer\">William Gay</div>\n            <div id=\"energybar\" style=\"height: 100%; width: 100%; background: #00FFFF; border-radius: 2px;\">\n            </div>\n            \n        </div>\n\n        \n        <script src=\"pulling.js\"></script>\n    </body>\n</html>";
        response = ResponseGen("200 OK", "text/html", Overlay);
    }
    ROUTE_GET("/overlay/reselim") {
    OverlayReselim:
        std::string Overlay = "<html>\n    <head>\n        <title>Streamer Tools Viewer Quest Overlay</title>\n        <meta property=\"og:site_name\" content=\"ComputerElite\">\n        <meta property=\"og:title\" content=\"Streamer Tools Viewer Quest Overlay\" />\n        <meta property=\"og:description\" content=\"An overlay for Streamer Tools Viewer for Beat Saber on Quest. Recreation of the one by reselim\" />\n        <meta property=\"og:url\" content=\"https://computerelite.github.io/tools/Streamer_Tools_Quest_Overlay/Overlay2.html\" />\n        <link href=\'https://fonts.googleapis.com/css?family=Open+Sans:400,400italic,700,700italic\' rel=\'stylesheet\' type=\'text/css\'>\n        <link href=\"https://computerelite.github.io/css/standard.css\" type=\"text/css\" rel=\"stylesheet\">\n        <link rel=\"icon\" href=\"../../assets/CE_64px.png\" type=\"image/x-icon\">\n        <style>\n            body {\n                background-color: #000000FF;\n                background-image: none;\n                padding: 20px;\n                letter-spacing: 2px;\n            }\n        </style>\n    </head>\n    <body>\n        <div style=\"display: flex; margin-bottom: 10px;\">\n            <div style=\"flex: 1; position: relative;\">\n                <img style=\"width: 100px; height: 100px;\" src=\"https://computerelite.github.io/tools/Streamer_Tools_Quest_Overlay/default.png\" id=\"cover\">\n                <svg style=\"position: absolute; width: 100px; height: 100px; left: 0px; top: 0px; transform: rotate(-90deg)\">\n                    <circle cx=\"50\" cy=\"50\" r=\"45\" opacity=\"0.3\" stroke-width=\"6\" fill=\"#222222\"></circle>\n                    <circle cx=\"50\" cy=\"50\" r=\"45\" stroke=\"#666666\" opacity=\"0.8\" stroke-width=\"6\" fill=\"transparent\"></circle>\n                    <circle cx=\"50\" cy=\"50\" r=\"45\" stroke=\"#FFFFFF\" opacity=\"1.0\" stroke-width=\"4\" id=\"progress\" fill=\"transparent\" stroke-dasharray=282 stroke-dashoffset=280></circle>\n                </svg>\n                <div id=\"timePlayed\" style=\"position: absolute; width: 100%; height: 100%; text-align: center; top: 40px;\">3:14</div>\n            </div>\n            <div style=\"flex: 16; margin-left: 10px;\">\n                <div style=\"font-size: 170%; font-weight: bold; display: inline;\"><div style=\"font-size: 100%; font-weight: bold; display: inline;\" id=\"songName\">Song name</div> (<div style=\"font-size: 100%; font-weight: bold; display: inline;\" id=\"songSub\"></div>)</div>\n                <div style=\"font-size: 125%;\"><div style=\"font-size: 100%; display: inline;\" id=\"songAuthor\">Song Author</div> [<div style=\"font-size: 100%; display: inline;\" id=\"mapper\">Mapper</div>]</div>\n                <div style=\"margin-top: 5px;\">\n                    <div style=\"font-size: 110%; background-color: #EEEEEE; color: #000000; border-radius: 4px; padding: 1px; padding-left: 5px; padding-right: 5px; margin-top: 20px; width: min-content; font-weight: bold; display: inline;\" id=\"diff\">diff</div>\n                    <div style=\"display: inline; font-size: 87%;\"><div style=\"display: inline;\" id=\"bpm\"></div> BPM</div>\n                    <div style=\"display: inline; font-size: 87%;\"><div style=\"display: inline;\" id=\"njs\"></div> NJS</div>\n                </div>\n                <div id=\"mpCodeContainer\" style=\"font-size: 80%; margin-top: 3px;\">Mp lobby: <div style=\"display: inline;\" id=\"mpCode\">*****</div></div>\n            </div>\n        </div>\n        <div style=\"font-size: 190%; font-weight: bold;\" id=\"score\">Score</div>\n        <div style=\"font-size: 130%; display: inline;\" id=\"combo\">Combo</div><div style=\"display: inline; font-size: 90%; color: #999999; margin-left: 5px;\">Combo</div>\n        <br/>\n        <div style=\"font-size: 130%; display: inline;\" id=\"rank\">Rank</div><div style=\"display: inline; font-size: 90%; color: #999999; margin-left: 5px;\" id=\"percentage\"></div>\n        <br/>\n        <div style=\"display: inline; font-size: 100%; color: #999999;\" id=\"customText\"></div>\n\n        <div style=\"font-size: 5px; position: absolute; top: 0px; right: 0px;\" id=\"williamGayContainer\">William Gay</div>\n        <div style=\"font-size: 5px; position: absolute; bottom: 0px; right: 0px;\" id=\"pinkCuteContainer\">Pink Cute</div>\n        <div style=\"font-size: 5px; position: absolute; bottom: 0px; right: 50px;\" id=\"eraCuteContainer\">Era Cute</div>\n        <script src=\"pulling.js\"></script>\n        <script>\n            function updateTime(total, played) {\n                timePlayed.innerHTML = ToElapsed(played)\n                document.getElementById(\"progress\").style.strokeDashoffset = 282 - played / total * 282\n            }\n        </script>\n    </body>\n</html>";
        response = ResponseGen("200 OK", "text/html", Overlay);
    }
    ROUTE_GET("/overlay/bandoot") {
    OverlayBandoot:
        std::string Overlay = "<html>\n    <head>\n        <title>Streamer Tools Viewer Quest Overlay</title>\n        <meta property=\"og:site_name\" content=\"ComputerElite\">\n        <meta property=\"og:title\" content=\"Streamer Tools Viewer Quest Overlay\" />\n        <meta property=\"og:description\" content=\"An overlay for Streamer Tools Viewer for Beat Saber on Quest. Recreation of Bandoots one\" />\n        <meta property=\"og:url\" content=\"https://computerelite.github.io/tools/Streamer_Tools_Quest_Overlay/Overlay3.html\" />\n        <link href=\'https://fonts.googleapis.com/css?family=Open+Sans:400,400italic,700,700italic\' rel=\'stylesheet\' type=\'text/css\'>\n        <link href=\"https://computerelite.github.io/css/standard.css\" type=\"text/css\" rel=\"stylesheet\">\n        <link rel=\"icon\" href=\"../../assets/CE_64px.png\" type=\"image/x-icon\">\n        <style>\n            body {\n                background-color: #00000000;\n                background-image: none;\n                padding: 20px;\n                letter-spacing: 2px;\n            }\n\n            .item {\n                background-color: #33333388;\n                border-radius: 5px;\n                padding: 3px;\n                margin-top: 5px;\n                width: min-content;\n                font-size: 100%;\n            }\n        </style>\n    </head>\n    <body>\n        <div style=\"display: flex; margin-bottom: 10px; position: absolute; bottom: 10px; left: 10px; flex-direction: column-reverse;\">\n            <div style=\"display: flex; position: absolute; bottom: 10px; left: 10px; flex: 1;\">\n                <div style=\"flex: 1;\">\n                    <img style=\"bottom: 0px; position: absolute; width: 150px; height: 150px; border-radius: 10px;\" src=\"https://computerelite.github.io/tools/Streamer_Tools_Quest_Overlay/default.png\" id=\"cover\">\n                </div>\n                <div style=\"flex: 1; flex-shrink: 0; white-space: nowrap; margin-left: 160px;\">\n                    <div class=\"item\" style=\"font-size: 120%; font-weight: bold; display: inline;\">BSR: <div style=\"display: inline;\" id=\"key\"></div></div>\n                    <div class=\"item\" style=\"font-size: 120%; font-weight: bold;\" id=\"mapper\">Mapper</div>\n                    <div class=\"item\" style=\"font-size: 130%; font-weight: bold;\" id=\"songAuthor\">Song Author</div>\n                    <div class=\"item\" style=\"font-size: 190%; font-weight: bold;\" id=\"songName\">Song name</div>\n                </div>\n            </div>\n            <div style=\"flex: 1; margin-bottom: 165px; white-space: nowrap; font-size: 16px; display: inline; margin-left: 10px; font-weight: bold;\">\n                <div class=\"item\" id=\"customText\" style=\"display: inline;\"></div>\n                <div id=\"mpCodeContainer\" class=\"item\">Mp lobby: <div id=\"mpCode\" style=\"display: inline;\"></div></div>\n                <div class=\"item\" id=\"preKeyContainer\">Pre-BSR: <div id=\"preKey\" style=\"display: inline;\"></div></div>\n                <div class=\"item\"style=\"font-size: 5px; padding: 2px; border-radius: 2px;\" id=\"williamGayContainer\">William Gay</div>\n                <div class=\"item\" style=\"font-size: 5px; padding: 2px; border-radius: 2px; \" id=\"pinkCuteContainer\">Pink Cute</div>\n            </div>\n        </div>\n        <div style=\"position: absolute; top: -5px; right: 0px; left: 0px; background: #33333388; margin: auto; width: min-content; padding: 5px; border-radius: 5px;\">\n            <div style=\"display: flex; text-align: center; white-space: nowrap; font-weight: bold; font-size: 20px;\">\n                <div style=\"flex: 1;\" style=\"display: inline;\">\n                    <div id=\"timePlayed\" style=\"display: inline;\"></div>/<div id=\"totalTime\" style=\"display: inline;\"></div>\n                </div>\n                <div style=\"flex: 1; margin-left: 20px;\">\n                    <div id=\"score\"></div>\n                </div>\n                <div style=\"flex: 1; margin-left: 20px;\">\n                    <div id=\"percentage\"></div>\n                </div>\n                <div style=\"flex: 1; margin-left: 20px;\">\n                    <div id=\"combo\"></div>\n                </div>\n                <div style=\"flex: 1; margin-left: 20px;\">\n                    <div style=\"font-size: 5px; position: absolute; bottom: 0px; right: 50px; padding: 2px; border-radius: 2px; \" id=\"eraCuteContainer\">Era Cute</div>\n                </div>\n            </div>\n        </div>\n        <div id=\"energybarContainer\" style=\"width: 30%; height: 2%; position: absolute; background-color: #33333388; border-radius: 3px; margin: auto; top: 0px; left: 100px; padding: 1px;\">\n            <div id=\"energybar\" style=\"height: 100%; width: 100%; background: #EEEEEE; border-radius: 2px;\">\n            </div>\n        </div>\n        <script src=\"pulling.js\"></script>\n        <script>\n            function updateTime(total, played) {\n                timePlayed.innerHTML = ToElapsed(played)\n                totalTime.innerHTML = ToElapsed(total)\n            }\n        </script>\n        \n    </body>\n</html>";
        response = ResponseGen("200 OK", "text/html", Overlay);
    }
    // Does not work
    //ROUTE_GET("/OverlayIframe") {
    //    response = ResponseGen("200 OK","text/html",
    //        "<!DOCTYPE html> "\
    //        "<html> "\
    //        "<head> <title>streamer-tools - Embedded Overlay</title> "\
    //        "<link href='https://fonts.googleapis.com/css?family=Open+Sans:400,400italic,700,700italic' rel='stylesheet' type='text/css'> "\
    //        "</head> "\
    //        "<iframe src=\"http://computerelite.github.io/tools/Streamer_Tools_Quest_Overlay/Overlay2.html?ip=" + STManager::localIP + "&updaterate=100&decimals=" + std::to_string(getModConfig().DecimalsForNumbers.GetValue()) +"\"></iframe>" \
    //        "</html>");
    //}
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