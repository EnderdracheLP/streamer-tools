#define RAPIDJSON_HAS_STDSTRING 1 // Enable rapidjson's support for std::string
#define NO_CODEGEN_USE

#include "ServerHeaders.hpp"
#include "STmanager.hpp"

bool STManager::runServer() {
    // Make our IPv4 endpoint
    sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(PORT);
    addr.sin_addr.s_addr = inet_addr(ADDRESS);

    int sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP); // Create the socket
    // Prevents the socket taking a while to close from causing a crash
    int iSetOption = 1;
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (char*)&iSetOption, sizeof(iSetOption));
    if (sock == -1) {
        logger.error("Error creating socket: %s", strerror(errno));
        return false;
    }

    // Attempt to bind to our port
    if (bind(sock, (struct sockaddr*)&addr, sizeof(addr))) {
        logger.error("Error binding to port %d: %s", PORT, strerror(errno));
        close(sock);
        return false;
    }

    logger.info("Listening on port %d", PORT);
    while (true) {
        if (listen(sock, CONNECTION_QUEUE_LENGTH) == -1) { // Return if an error occurs listening for a request
            logger.error("Error listening for request");
            continue;
        }

        socklen_t socklen = sizeof addr;
        int client_sock = accept(sock, (struct sockaddr*)&addr, &socklen); // Accept the next request
        if (client_sock == -1) {
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