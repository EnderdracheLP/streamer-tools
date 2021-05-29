#define RAPIDJSON_HAS_STDSTRING 1 // Enable rapidjson's support for std::string
#define NO_CODEGEN_USE

#include "ServerHeaders.hpp"
#include "STmanager.hpp"

//LoggerContextObject MulticastLogger;

bool STManager::MulticastServer() {
    MulticastRunning = true;
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
        MulticastRunning = false;
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
            MulticastRunning = false;
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
        logger.error("Multicast: setting local interface: %s", strerror(errno));
        MulticastRunning = false;
        return false;
    }

    struct ifreq ifr;
    while (true) {
        std::chrono::minutes MulticastSleep(1);
        while (!ConnectedHTTP && !ConnectedSocket) {
            std::chrono::seconds timespan(15);

            /* I want to get an IPv4 IP address */
            ifr.ifr_addr.sa_family = AF_INET;

            /* I want IP address attached to "wlan0" */
            strncpy(ifr.ifr_name, "wlan0", IFNAMSIZ - 1);

            ioctl(sd, SIOCGIFADDR, &ifr);

            /*
            * Create message to be sent
            */
            //    std::string message = inet_ntoa(((struct sockaddr_in*)&ifr.ifr_addr)->sin_addr);
            STManager::localIp = inet_ntoa(((struct sockaddr_in*)&ifr.ifr_addr)->sin_addr);
            std::string http = STManager::localIp + ":" + std::to_string(PORT_HTTP);
            std::string socket = STManager::localIp + ":" + std::to_string(PORT);
            std::string message = multicastResponse(socket, http);

            /*
             * Send a message to the multicast group specified by the
             * groupSock sockaddr structure.
             */
            
            MulticastLogger.debug("Multicast sent: \n%s", message.c_str());
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
        std::this_thread::sleep_for(MulticastSleep);
    }
    MulticastRunning = false;
    return true;
}