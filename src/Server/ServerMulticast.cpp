#define RAPIDJSON_HAS_STDSTRING 1 // Enable rapidjson's support for std::string
#define NO_CODEGEN_USE

#include "ServerHeaders.hpp"
#include "STmanager.hpp"
#include <ifaddrs.h>
#include <netdb.h>

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
        logger.error("Error opening datagram socket");
        MulticastRunning = false;
        return false;
    }

    /*
     * Initialize the group sockaddr structure
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
            logger.error("Error setting IP_MULTICAST_LOOP: %s", strerror(errno));
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
    localInterface.s_addr = inet_addr("0.0.0.0");
    if (setsockopt(sd, IPPROTO_IP, IP_MULTICAST_IF,
        (char*)&localInterface,
        sizeof(localInterface)) < 0) {
        logger.error("Error Multicast: setting local interface: %s", strerror(errno));
        MulticastRunning = false;
        return false;
    }

    struct ifreq ifr;

    struct ifaddrs* ifap, * ifa;
    struct sockaddr_in6* sa;
    char addrIPv6[INET6_ADDRSTRLEN];

    std::string addressIPv6Check;
    std::string addressIPv6;

    if (getifaddrs(&ifap) == -1) {
        logger.error("getifaddrs");
    }

    for (ifa = ifap; ifa; ifa = ifa->ifa_next) {
        if (ifa->ifa_addr && ifa->ifa_addr->sa_family == AF_INET6) {
            sa = (struct sockaddr_in6*)ifa->ifa_addr;
            getnameinfo(ifa->ifa_addr, sizeof(struct sockaddr_in6), addrIPv6,
                sizeof(addrIPv6), NULL, 0, NI_NUMERICHOST);
            //logger.debug("Interface: %s\tAddress: %s\n", ifa->ifa_name, addrIPv6);
            // This will get the first IPv6 address on the wlan0 interface that is not link-local
            if (strcmp(ifa->ifa_name, "wlan0") == 0) {
                addressIPv6Check = addrIPv6;
                if (!addressIPv6Check.starts_with("fe80") && !addressIPv6Check.empty()) {
                    addressIPv6 = addressIPv6Check;
                    break;
                }
            }
        }
    }

    freeifaddrs(ifap);

    /* I want to get an IPv4 IP address */
    ifr.ifr_addr.sa_family = AF_INET;

    /* I want IP address attached to "wlan0" */
    strncpy(ifr.ifr_name, "wlan0", IFNAMSIZ - 1);

    ioctl(sd, SIOCGIFADDR, &ifr);

    char numeric_addr_v4[INET_ADDRSTRLEN];

    STManager::localIP = inet_ntop(AF_INET, &((struct sockaddr_in*)&ifr.ifr_addr)->sin_addr, numeric_addr_v4, sizeof numeric_addr_v4);
    STManager::localIPv6 = addressIPv6;

    while (true) {
        std::chrono::minutes MulticastSleep(1);
        while (!ConnectedHTTP && !ConnectedSocket) {
            std::chrono::seconds timespan(15);

            /*
            * Create message to be sent
            */
           
            std::string message = multicastResponse();

            /*
             * Send a message to the multicast group specified by the
             * groupSock sockaddr structure.
             */
            
            LOG_DEBUG_MULTICAST("Multicast sent: \n%s", message.c_str());
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
