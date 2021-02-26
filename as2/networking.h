#ifndef _NETWORKING_H_
#define _NETWORKING_H_

#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>

#define MSG_MAX_LEN 20000
#define PORT 12345

//Module for communicatino between client and host via UDP.

int socketDescriptor;
// Address
struct sockaddr_in server, client;

// send and recieve functions
void Networking_sendPacket(char *messageTx);
void Networking_recievePacket(char *messageRx);
// startup and shutdown functions
int Networking_configNetwork();
void Networking_shutDownNetwork();

#endif