#ifndef _NETWORKING_H_
#define _NETWORKING_H_

#define MSG_MAX_LEN 1024
#define PORT 12345

int socketDescriptor;
// Address
struct sockaddr_in server;
struct sockaddr_in client;

void Networking_sendPacket(char message);
char Networking_recievePacket();
int Networking_configNetwork();
void Networking_shutDownNetwork();

#endif