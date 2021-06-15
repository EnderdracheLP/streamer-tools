#pragma once
#include <arpa/inet.h>
#include <sys/socket.h>
#include <net/if.h>

#define ADDRESS_MULTI "232.0.53.5"  // Testing setting was "225.1.1.1" and "224.0.0.1" which sends to all hosts on the network
#define PORT_MULTI 53500
#define PORT 53501
#define PORT_HTTP 53502
#define CONNECTION_QUEUE_LENGTH 2 // How many connections to store to process