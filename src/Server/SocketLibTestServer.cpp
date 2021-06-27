#define RAPIDJSON_HAS_STDSTRING 1 // Enable rapidjson's support for std::string
#define NO_CODEGEN_USE
#include "STmanager.hpp"
#include "Config.hpp"

#include "UnityEngine/ImageConversion.hpp"
#include "UnityEngine/Texture2D.hpp"
#include "Unity/Collections/NativeArray_1.hpp"
#include "System/Convert.hpp"

#include "socket_lib/shared/SocketHandler.hpp"
using namespace SocketLib;

#define PORT_HTTP 53502
//#define PORT_HTTP 53520


//void STManager::connectEvent(int clientDescriptor, bool connected) {
//    //std::cout << "Connected " << clientDescriptor << " status: " << (connected ? "true" : "false") << std::endl;
//    getLogger().debug("Connected %d status: %s", clientDescriptor, connected ? "true" : "false");
//    if (connected) {
//        std::string response;
//        if (!coverTexture) {
//            response = ResponseGen("404 Not Found", "text/html",
//                "<!DOCTYPE html> "\
//                "<html> "\
//                "<head> <title>streamer-tools - 404 Not found</title> "\
//                "<link href='https://fonts.googleapis.com/css?family=Open+Sans:400,400italic,700,700italic' rel='stylesheet' type='text/css'> "\
//                "<style> body { color: #EEEEEE; background-color: #202225; font-size: 14px; font-family: 'Open Sans'; } </style> "\
//                "</head> "\
//                "<body> <div style=\"font-size: 30px;\">Streamer-tools - 404 Not found</div> "\
//                "<div style=\"color: #888; margin-bottom: 10px; padding-left: 20px;\">The endpoint you were looking for could not be found.</div> "\
//                "<div style=\"font-size: 18px; margin-top: 30px; border-top: solid #BBBBBB 2px; padding: 10px; width: fit-content;\"><i>" + STModInfo.id + "/" + STModInfo.version + " (" + headsetType + ") server at " + STManager::localIP + ":" + std::to_string(53520) + "</i></div> "\
//                "</body> </html>"); // Yes this is long but page is pretty-ish
//        }
//        else if (CoverChanged[3]) {
//            coverImagePNG = GetCoverImage("png", false);
//            CoverChanged[3] = false;
//            response = ResponseGen("200 OK", "image/png", /*stats*/ coverImagePNG);
//        }
//        else LOG_DEBUG_HTTP("CoverImageUnchanged");
//
//
//        serverSocket->write(clientDescriptor, Message(response));
//    }
//}

void STManager::startHTTPserver() {
    getLogger().debug("Starting server at port " + std::to_string(PORT_HTTP));
    SocketHandler& socketHandler = SocketHandler::getCommonSocketHandler();

    serverSocket = socketHandler.createServerSocket(PORT_HTTP);
    serverSocket->bindAndListen();
    getLogger().debug("Started server");

    ServerSocket& serverSocket = *this->serverSocket;

    serverSocket.bufferSize = 4096 + 1;
    //serverSocket.registerConnectCallback([this](int client, bool connected) {
    //    connectEvent(client, connected);
    //    });

    serverSocket.registerListenCallback([this](Channel& client, const Message& message) {
        listenOnEvents(client, message);
        });

    getLogger().debug("Listening server fully started up");
    //while (serverSocket.isActive()) {}

    ////std::cout << "Finished server test, awaiting for server shutdown" << std::endl;
    //getLogger().debug("Finished server test, awaiting for server shutdown");


    // The destructor will remove the socket from the SocketHandler automatically.
    // You can use a smart pointer like unique pointer to handle this for you
    //delete this->serverSocket;
}

bool STManager::HandleConfigChangeSL(Channel& client, std::string message) {
    size_t start = message.find("{");
    LOG_DEBUG_HTTP("index of {: " + std::to_string(start));
    size_t end = message.find("}");
    LOG_DEBUG_HTTP("index of }: " + std::to_string(end));
    std::string json = message.substr(start, end - start + 1);
    LOG_DEBUG_HTTP("json: " + json);
    rapidjson::Document document;
    if (document.Parse(json).HasParseError()) {
        client.queueWrite(Message(ResponseGen("400 Bad Request", "text/html",
            "<!DOCTYPE html> "\
            "<html> "\
            "<head> <title>streamer-tools - 400 Bad Request</title> "\
            "<link href='https://fonts.googleapis.com/css?family=Open+Sans:400,400italic,700,700italic' rel='stylesheet' type='text/css'> "\
            "<style> body { color: #EEEEEE; background-color: #202225; font-size: 14px; font-family: 'Open Sans'; } </style> "\
            "</head> "\
            "<body> <div style=\"font-size: 30px;\">Streamer-tools - 400 Bad Request</div> "\
            "<div style=\"color: #888; margin-bottom: 10px; padding-left: 20px;\">The request the Server received was invalid.</div> "\
            "<div style=\"font-size: 18px; margin-top: 30px; border-top: solid #BBBBBB 2px; padding: 10px; width: fit-content;\"><i>" + STModInfo.id + "/" + STModInfo.version + " (" + headsetType + ") server at " + STManager::localIP + ":" + std::to_string(PORT_HTTP) + "</i></div> "\
            "</body> </html>"))); // Yes this is long but page is pretty-ish));
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
            for (int i = 0; i < sizeof(BoolModConfigValueNames) / sizeof(char*); ++i) {
                if (document.FindMember(BoolModConfigValueNames[i]) != document.MemberEnd() && document[BoolModConfigValueNames[i]].IsBool()) {
                    CUBoolModConfigValueNames[i]->SetValue(document[BoolModConfigValueNames[i]].GetBool());
                    didValueChange = true;
                }
            }
            if (didValueChange) {
                getModConfig().LastChanged.SetValue(document["lastChanged"].GetInt());
                MakeConfigUI(true);

                client.queueWrite(Message(ResponseGen("200 OK", "application/json", constructConfigResponse(), "GET, PATCH")));
                return true;
            }
            else return false;
        }
        else {
            client.queueWrite(Message(ResponseGen("422 Unprocessable Entity", "text/html",
                "<!DOCTYPE html> "\
                "<html> "\
                "<head> <title>streamer-tools - 422 Unprocessable Entity</title> "\
                "<link href='https://fonts.googleapis.com/css?family=Open+Sans:400,400italic,700,700italic' rel='stylesheet' type='text/css'> "\
                "<style> body { color: #EEEEEE; background-color: #202225; font-size: 14px; font-family: 'Open Sans'; } </style> "\
                "</head> "\
                "<body> <div style=\"font-size: 30px;\">Streamer-tools - 422 Unprocessable Entity</div> "\
                "<div style=\"color: #888; margin-bottom: 10px; padding-left: 20px;\">The request the Server received contained an entity that could not be processed.</div> "\
                "<div style=\"font-size: 18px; margin-top: 30px; border-top: solid #BBBBBB 2px; padding: 10px; width: fit-content;\"><i>" + STModInfo.id + "/" + STModInfo.version + " (" + headsetType + ") server at " + STManager::localIP + ":" + std::to_string(PORT_HTTP) + "</i></div> "\
                "</body> </html>"))); // Yes this is long but page is pretty-ish));
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

#define ROUTE_START()       if ((msgStr.find("GET / ") != std::string::npos) || (msgStr.find("GET /index") != std::string::npos))
#define ROUTE(METHOD,URI)    if ((msgStr.find(METHOD " " URI " ") != std::string::npos) ||  (msgStr.find(METHOD " " URI "/ ") != std::string::npos) ||  (msgStr.find(METHOD " " URI "?") != std::string::npos))
#define ROUTE_PATH(PATH)    else if (msgStr.find(PATH" ") == std::string::npos && msgStr.find(PATH) != std::string::npos)
#define ROUTE_GET(URI)      ROUTE("GET", URI)
#define ROUTE_POST(URI)      ROUTE("POST", URI)
#define ROUTE_PUT(URI)      ROUTE("PUT", URI)
#define ROUTE_PATCH(URI)      ROUTE("PATCH", URI)


void STManager::listenOnEvents(Channel& client, const Message& message) {
    auto msgStr = message.toString();
    std::string response;
    std::string messageStr;
    LOG_SLIBHTTP("Received message is: %s", msgStr.c_str());
    ROUTE_START() {
        ROUTE_GET("/index.json") goto NormalResponse;
        //if (BufferStr);
        response = ResponseGen("200 OK", "text/html", "<!DOCTYPE html>\n<html>\n    <head>\n        <title>streamer-tools</title>\n        <link href=\'https://fonts.googleapis.com/css?family=Open+Sans:400,400italic,700,700italic\' rel=\'stylesheet\' type=\'text/css\'>\n        <style>\n            body {\n                color: #EEEEEE;\n                background-color: #202225;\n                font-size: 14px;\n                font-family: \'Open Sans\';\n            }\n            a {\n                color: #30A2EC;\n            }\n        </style>\n    </head>\n    <body>\n        <div style=\"font-size: 30px;\">Streamer-tools</div>\n        <div style=\"color: #EEEEEE; margin-bottom: 10px; padding-left: 20px;\">\n            Enhance your Beat Saber stream with overlays and more. This mod provides all data necessary for the client on your PC to work.\n            <br/>\n            Get <a href=\"https://github.com/ComputerElite/streamer-tools-client/\">the client by ComputerElite</a> or <a href=\"overlays.html\">Open the overlays in your browser</a> (not every overlay may work) to start your Beat Saber streamer life\n        </div>\n        <div style=\"font-size: 18px; margin-top: 30px; border-top: solid #BBBBBB 2px; padding: 10px; width: fit-content;\"><i>" + STModInfo.id + "/" + STModInfo.version + " (" + headsetType + ") server at " + STManager::localIP + ":" + std::to_string(PORT_HTTP) + "</i></div>\n    </body>\n</html>");
        client.queueWrite(Message(response));
        return;
    }
    ROUTE_GET("/data") {
    NormalResponse:
        messageStr = constructResponse();
        response = ResponseGen("200 OK", "application/json", messageStr);
        client.queueWrite(Message(response));
        return;
    }
    ROUTE_PATH("/cover/") {
        if (CoverStatus == Running) {
            std::unique_lock<std::mutex> lk(STManager::CoverLock);
            STManager::cv.wait_for(lk, std::chrono::seconds(2));
        }
        ROUTE_GET("/cover/base64") {
        COVER_B64_JPG:
            if (!coverTexture) goto NotFound;
            else if (CoverChanged[0]) {
                coverImageBase64 = GetCoverImage();
                CoverChanged[0] = false;
            }
            else LOG_DEBUG_HTTP("CoverImageUnchanged");

            response = ResponseGen("200 OK", "text/plain", coverImageBase64);
            client.queueWrite(Message(response));
            return;
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
            client.queueWrite(Message(response));
            return;
        }
        ROUTE_GET("/cover/cover.jpg") {
            if (!coverTexture) goto NotFound;
            else if (CoverChanged[2]) {
                coverImageJPG = GetCoverImage("jpg", false);
                CoverChanged[2] = false;
            }
            else LOG_DEBUG_HTTP("CoverImageUnchanged");

            response = ResponseGen("200 OK", "image/jpg", /*stats*/ coverImageJPG);
            client.queueWrite(Message(response));
            return;
        }
        ROUTE_GET("/cover/cover.png") {
            if (!coverTexture) goto NotFound;
            else if (CoverChanged[3]) {
                coverImagePNG = GetCoverImage("png", false);
                CoverChanged[3] = false;
            }
            else LOG_DEBUG_HTTP("CoverImageUnchanged");

            response = ResponseGen("200 OK", "image/png", /*stats*/ coverImagePNG);
            client.queueWrite(Message(response));
            return;
        }
    }
    ROUTE_GET("/config") {
    CONFIG:
        configFetched = true;
        response = ResponseGen("200 OK", "application/json", constructConfigResponse(), "GET, PATCH");
        client.queueWrite(Message(response));
        return;
    }
    ROUTE_PATCH("/config") {
    PCONFIG:
        if (!HandleConfigChangeSL(client, msgStr)) client.queueWrite(Message(ResponseGen("204")));
        return;
    }
    ROUTE_GET("/positions") {
        if (STManager::Head && STManager::VR_Right && STManager::VR_Left && !(location == 0 || location >= 4)) {
            client.queueWrite(Message(response));
            return;
        }
        else {
            client.queueWrite(Message(response));
            return;
        }
    }
    ROUTE_GET("/loader.html") {
        LOG_SLIBHTTP("In /loader.html");
        response = ResponseGen("200 OK", "text/html", "<!DOCTYPE html>\n<html>\n    <head>\n        <title>test</title>\n    </head>\n    <body>\n        Yeah so it load Something\n        <script>\n            var url = new URL(window.location.href);\n            var uri = url.searchParams.get(\"uri\");\n            var html = MakeTextGetRequest(uri)\n            var baseDir = uri\n            if(baseDir.endsWith(\"/\")) baseDir = baseDir.substring(0, baseDir.length - 1)\n            baseDir = baseDir.substring(0, baseDir.lastIndexOf(\"/\"))\n            const reg = /\"(((\\.\\.\\/)+)|\\/)?[a-zA-Z0-9\\-_\\.\\/]+\"/g\n            while ((match = reg.exec(html)) !== null) {\n                match[0] = match[0].substring(1, match[0].length - 1)\n                if(!match[0].includes(\".\") && !match[0].endsWith(\"/\")) continue;\n                console.log(`Found ${match[0]} start=${match.index} end=${reg.lastIndex}.`);\n                var replacement = GetAbsoluteLink(baseDir, match[0])\n                console.log(\"replacing with \" + replacement)\n                html = html.replace(match[0], replacement)\n            }\n\n            \n\n            const scriptReg = /<script( src=\".+?\")?>.*?<\\/script>/gs\n            const headStart = html.indexOf(\"<head>\") + 6\n            var scripts = []\n            while ((match = scriptReg.exec(html)) !== null) {\n                html = html.replace(match[0], \"\")\n                scripts.push(match[0]);\n                console.log(\"moved script \" + match[0] + \" into head\")\n            }\n\n            \n\n            document.documentElement.innerHTML = html\n            const srcRegex = /src=\".+?\"/gs\n            scripts.forEach(e => {\n                var script = document.createElement(\"script\")\n                if(e.substring(e.indexOf(\"<\"), e.indexOf(\">\")).includes(\"src\"))\n                {\n                    var found = srcRegex.exec(e.substring(0, e.indexOf(\">\")))[0];\n                    script.src = found.substring(5, found.length - 1)\n                } else script.appendChild(document.createTextNode(e.substring(e.indexOf(\">\") + 1, e.lastIndexOf(\"<\") - 1)))\n                document.head.appendChild(script)\n            })\n\n            function GetAbsoluteLink(baseUri, relativeUri) {\n                var absolute = baseUri;\n                if(relativeUri.startsWith(\"../\")) {\n                    var tmp = absolute.split(\"/\")\n                    tmp.pop();\n                    absolute = tmp.join(\"/\")\n                    absolute = GetAbsoluteLink(absolute, relativeUri.substring(3, relativeUri.length))\n                } else {\n                    absolute += \"/\" + relativeUri\n                }\n                return absolute\n            }\n\n            function InsertString(toInsert, position, text) {\n                return [text.slice(0, position), toInsert, text.slice(position)].join(\'\')\n            }\n\n            function MakeTextGetRequest(url) {\n                var request = new XMLHttpRequest();\n                request.open(\'GET\', url, false);\n                request.send(null);\n                if (request.status == 200) {\n                    return request.responseText;\n                } else {\n                    return \"Something went wrong: \" + request.status;\n                }\n            }\n        </script>\n    </body>\n</html>");
        LOG_SLIBHTTP("/loader.html sending response");
        client.queueWrite(Message(response));
        LOG_SLIBHTTP("/loader.html response sent");
        return;
    }
    ROUTE_GET("/overlays.html") {
        response = ResponseGen("200 OK", "text/html", "<!DOCTYPE html>\n<html>\n    <head>\n        <title>streamer-tools</title>\n        <link href=\'https://fonts.googleapis.com/css?family=Open+Sans:400,400italic,700,700italic\' rel=\'stylesheet\' type=\'text/css\'>\n        <style>\n            body {\n                color: #EEEEEE;\n                background-color: #202225;\n                font-size: 14px;\n                font-family: \'Open Sans\';\n            }\n            a {\n                color: #30A2EC;\n            }\n        </style>\n    </head>\n    <body>\n        <div style=\"font-size: 30px;\">Streamer-tools - Overlays</div>\n        <div style=\"color: #EEEEEE; margin-bottom: 10px; padding-left: 20px;\">\n            Here are all currently available overlays (not every overlay may work):\n            <br/>\n            <div id=\"overlays\">\n\n            </div>\n        </div>\n        <div style=\"font-size: 18px; margin-top: 30px; border-top: solid #BBBBBB 2px; padding: 10px; width: fit-content;\"><i>" + STModInfo.id + "/" + STModInfo.version + " (" + headsetType + ") server at " + STManager::localIP + ":" + std::to_string(PORT_HTTP) + "</i></div>\n        <script>\n        const baseUrl = window.location.href.split(\"?\")[0];\n        function FormatToHTML(overlay) {\n            var link = \"loader.html?uri=\" + getLink(overlay) + \"&ip=" + STManager::localIP + "\"\n            return `<div style=\"margin-top: 20px;\">${overlay.Name}:<br/><div style=\"font-size: 14px;\">URL: <a style=\"font-size: 14px;\" target=\"_blank\" href=\"${link}\">${link}</a><br/><input type=\'button\' onclick=\'Copy(\"${link}\")\' style=\"margin-bottom: 5px;\" value=\"Copy URL\"><br/><iframe width=85% height=\"500px\" src=\"${link}\"></iframe></div></div>`\n        }\n\n        function getLink(overlay) {\n            for(let i = 0; i < overlay.downloads.length; i++) {\n                if(overlay.downloads[i].IsEntryPoint) return overlay.downloads[i].URL\n            }\n        }\n\n        fetch(`https://computerelite.github.io/tools/Streamer_Tools_Quest_Overlay/overlays.json`).then(res => {\n            res.json().then(json => {\n                document.getElementById(\"overlays\").innerHTML = \"\";\n                json.overlays.forEach(overlay => {\n                    document.getElementById(\"overlays\").innerHTML += FormatToHTML(overlay);\n                })\n            })\n        })\n        </script>\n    </body>\n</html>");
        client.queueWrite(Message(response));
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
        client.queueWrite(Message(response));
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
        "<div style=\"font-size: 18px; margin-top: 30px; border-top: solid #BBBBBB 2px; padding: 10px; width: fit-content;\"><i>" + STModInfo.id + "/" + STModInfo.version + " (" + headsetType + ") server at " + STManager::localIP + ":" + std::to_string(PORT_HTTP) + "</i></div> "\
        "</body> </html>"); // Yes this is long but page is pretty-ish
    client.queueWrite(Message(response));



        // This is not the recommended way to fully stop the server
        // this only sets the variable to false so the server socket can set active to false
        // which will release the loop and delete the pointer
        //serverSocket->notifyStop();
}

