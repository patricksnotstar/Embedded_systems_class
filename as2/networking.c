#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdbool.h>
#include <pthread.h>
#include <netdb.h>
#include <unistd.h> // for close()
#include <string.h>

#include "sorter.h"
#include "networking.h"

int Networking_configNetwork()
{

    memset(&server, 0, sizeof(server));
    memset(&client, 0, sizeof(client));

    server.sin_family = AF_INET;                // Connection may be from runningMutex
    server.sin_addr.s_addr = htonl(INADDR_ANY); // Host to Network long
    server.sin_port = htons(PORT);              // Host to Network short

    socketDescriptor = socket(PF_INET, SOCK_DGRAM, 0);
    if (socketDescriptor < 0)
    {
        perror("Could not create socket descriptor\n");
        return 1;
    }

    // Bind the socket to the port (PORT) that we specify
    bind(socketDescriptor, (struct sockaddr *)&server, sizeof(server));

    return 0;
}

void Networking_shutDownNetwork()
{
    close(socketDescriptor);
}

void Networking_sendPacket(char *messageTx)
{
    // Transmit a reply:
    unsigned int sin_len = sizeof(client);
    // char messageTx[MSG_MAX_LEN];
    // memset(messageTx, 0, sizeof(messageTx));
    // put message in messagTx
    // sprintf(messageTx, "%s", message);
    sendto(socketDescriptor,
           messageTx, strlen(messageTx),
           0,
           (const struct sockaddr *)&client, sin_len);
}

void Networking_recievePacket(char *messageRx)
{
    unsigned int sin_len = sizeof(client);
    int bytesRx = recvfrom(socketDescriptor,
                           messageRx, MSG_MAX_LEN, 0,
                           (struct sockaddr *)&client, &sin_len);

    // Make it null terminated (so string functions work):
    int terminateIdx = (bytesRx < MSG_MAX_LEN) ? bytesRx : MSG_MAX_LEN - 1;
    messageRx[terminateIdx] = 0;
    // printf("Message received (%d bytes): \n\n'%s'\n", bytesRx, messageRx);
}
