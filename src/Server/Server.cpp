#define RAPIDJSON_HAS_STDSTRING 1 // Enable rapidjson's support for std::string
#define NO_CODEGEN_USE

#include "ServerHeaders.hpp"
#include "STmanager.hpp"

bool STManager::runServer() {
    // Make our IPv6 endpoint
    sockaddr_in6 addr;
    addr.sin6_family = AF_INET6;
    addr.sin6_port = htons(PORT);
    addr.sin6_addr = in6addr_any;
    int v6OnlyEnabled = 0;
    char numeric_addr[INET6_ADDRSTRLEN];

    int sock = socket(AF_INET6, SOCK_STREAM, IPPROTO_TCP); // Create the socket
    // Prevents the socket taking a while to close from causing a crash
    int iSetOption = 1;
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (char*)&iSetOption, sizeof(iSetOption));
    setsockopt(sock, IPPROTO_IPV6, IPV6_V6ONLY, &v6OnlyEnabled, sizeof(v6OnlyEnabled)); // Disable v6 Only restriction to allow v4 connections
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
            ConnectedSocket = false;
            continue;
        }

        socklen_t socklen = sizeof addr;
        int client_sock = accept(sock, (struct sockaddr*)&addr, &socklen); // Accept the next request
        if (client_sock == -1) {
            logger.error("Error accepting request");
            ConnectedSocket = false;
            continue;
        }
        std::string ClientIP(inet_ntop(AF_INET6, (struct sockaddr*)&addr.sin6_addr, numeric_addr, sizeof numeric_addr));
        // If ClientIP starts with ffff it's an IPv4
        if (ClientIP.starts_with("::ffff:")) {
            ClientIP = ClientIP.substr(7, ClientIP.length());
        }
        logger.info("Client connected with address: %s", ClientIP.c_str());
        ConnectedSocket = true; // Set this to true here so it no longer sends out Multicast while a connection has been established.

        // Pass the socket handle over and start a seperate thread for sending back the reply
        std::thread RequestThread([this, client_sock]() { return STManager::sendResponse(client_sock); }); // Use threads for the response
        RequestThread.detach(); // Detach the thread from this thread
    }

    close(sock);
    return true;
}

void STManager::sendResponse(int client_sock) {
    while (client_sock != -1) {
        std::string response = constructResponse();
        LOG_DEBUG_SOCKET("Response: %s", response.c_str());

        int convertedLength = htonl(response.length());
        if (write(client_sock, &convertedLength, 4) == -1) { // First send the length of the data
            logger.error("Error sending length prefix: %s", strerror(errno));
            ConnectedSocket = false;
            close(client_sock); return;
        }
        if (write(client_sock, response.c_str(), response.length()) == -1) { // Then send the string
            logger.error("Error sending JSON: %s", strerror(errno));
            ConnectedSocket = false;
            close(client_sock); return;
        }
        std::chrono::milliseconds timespan(50);
        std::this_thread::sleep_for(timespan);
    }
    close(client_sock); // Close the client's socket to avoid leaking resources
    ConnectedSocket = false;
    return;
}