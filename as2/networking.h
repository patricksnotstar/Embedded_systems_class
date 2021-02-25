#ifndef _NETWORKING_H_
#define _NETWORKING_H_

#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>

#define MSG_MAX_LEN 20000
#define PORT 12345

int socketDescriptor;
// Address
struct sockaddr_in server, client;

void Networking_sendPacket(char *messageTx);
void Networking_recievePacket(char *messageRx);
int Networking_configNetwork();
void Networking_shutDownNetwork();

#endif