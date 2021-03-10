//Module for handling communication between client and host via UDP and shutting down network.
#ifndef _SERVER_H_
#define _SERVER_H_

#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>

#define MSG_MAX_LEN 20000
#define PORT 12345

int socketDescriptor;
// Address
struct sockaddr_in server, client;

// send and recieve functions
void Server_sendPacket(char *messageTx);
// startup and shutdown functions
int Server_configNetwork();
void Server_shutDownNetwork();

#endif