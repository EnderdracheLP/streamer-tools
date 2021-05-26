#define RAPIDJSON_HAS_STDSTRING 1 // Enable rapidjson's support for std::string
#define NO_CODEGEN_USE

#include "ServerHeaders.hpp"
#include "STmanager.hpp"

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
    if (resultStr.find("GET / ") != std::string::npos) {
        std::string stats = constructResponse();
        std::string response = "HTTP/1.1 200 OK\nContent-Length: " + std::to_string(stats.length()) + "\nContent-Type: application/json\nAccess-Control-Allow-Origin: *\n\n" + stats;
        logger.info("HTTP Response: \n%s", response.c_str());

        int convertedLength = htonl(response.length());
        if (write(client_sock, response.c_str(), response.length()) == -1) { // Then send the string
            logger.error("HTTP: Error sending JSON: \n%s", strerror(errno));
            close(client_sock); return;
        }
    }
    else if (resultStr.find("GET /cover/ ") != std::string::npos || resultStr.find("GET /cover ") != std::string::npos) {
        // To-Do: Send Playlist cover
        std::string stats = "<!DOCTYPE html> <html> <head> <title> No Cover image for you little guy or girl </title> </head> <body> Do you really think covers are implemented yet??? Also yeet that page it's uGlY </body> </html>";
        std::string response = "HTTP/1.1 200 OK\nContent-Length: " + std::to_string(stats.length()) + "\nContent-Type: text/html\nAccess-Control-Allow-Origin: *\n\n" + stats;
        logger.info("HTTP Response: \n%s", response.c_str());

        int convertedLength = htonl(response.length());
        if (write(client_sock, response.c_str(), response.length()) == -1) { // Then send the string
            logger.error("HTTP: Error sending JSON: \n%s", strerror(errno));
            close(client_sock); return;
        }
    }
    else {
        // 404 or invalid
        std::string messageError = "<!DOCTYPE html> <html> <head> <title>streamer-tools - 404 Not found</title> <link href='https://fonts.googleapis.com/css?family=Open+Sans:400,400italic,700,700italic' rel='stylesheet' type='text/css'> <style> body { color: #EEEEEE; background-color: #202225; font-size: 14px; font-family: 'Open Sans'; } </style> </head> <body> <div style=\"font-size: 30px;\">Streamer-tools - 404 Not found</div> <div style=\"color: #888; margin-bottom: 10px; padding-left: 20px;\">The endpoint you were looking for could not be found.</div> <div style=\"font-size: 18px; margin-top: 30px; border-top: solid #BBBBBB 2px; padding: 10px; width: fit-content;\"><i>" + STModInfo.id + "/" + STModInfo.version + " (Oculus Quest) server at ip " + STManager::localIp + ":" + std::to_string(PORT_HTTP) + "</i></div> </body> </html>"; // Yes this is long but page is pretty-ish
        std::string responseInvalid = "HTTP/1.1 404 Not Found\nContent-Length: " + std::to_string(messageError.length()) + "\nContent-Type: text/html\nAccess-Control-Allow-Origin: *\n\n" + messageError;
        logger.info("HTTP Not Found Response: %s", responseInvalid.c_str());

        int convertedLengthInvalid = htonl(responseInvalid.length());
        if (write(client_sock, responseInvalid.c_str(), responseInvalid.length()) == -1) { // Then send the string
            logger.error("HTTP: Error sending HTTP Response: %s", strerror(errno));
            close(client_sock); logger.debug("Received message: \n%s", resultStr.c_str()); return;
        }
        logger.error("HTTP: Response Status Code: %s", strerror(errno));
    }
    close(client_sock); // Close the client's socket to avoid leaking resources
    return;
}