#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdbool.h>
#include <pthread.h>
#include <netdb.h>
#include <unistd.h> // for close()
#include <string.h>

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

    // Make it null terminated
    int terminateIdx = (bytesRx < MSG_MAX_LEN) ? bytesRx : MSG_MAX_LEN - 1;
    messageRx[terminateIdx] = 0;
}

// Function to split packets to send using UDP
void Networking_splitPackets(char *outResponse, int outResponseSize)
{
    char packet[MAX_UDP_PACKET_SIZE];
    for (int i = 0; i < outResponseSize;)
    {
        memset(packet, '\0', MAX_UDP_PACKET_SIZE);
        int j = i;
        while (j < outResponseSize)
        {
            if (outResponse[j] != '\n')
            {
                j++;
            }
            else
            {
                break;
            }
        }
        strncpy(packet, outResponse + i, j - i + 1);
        i = j + 1;
        Networking_sendPacket(packet);
        long seconds = 0;
        long nanoseconds = 1000000000;
        struct timespec reqDelay = {seconds, nanoseconds};
        nanosleep(&reqDelay, (struct timespec *)NULL);
    }
}