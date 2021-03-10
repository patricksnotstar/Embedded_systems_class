#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdbool.h>
#include <pthread.h>
#include <netdb.h>
#include <unistd.h> // for close()
#include <string.h>

#include "server.h"
#include "helper.h"
#include "beatbox.h"
#include "audioMixer.h"

void *Server_thread();
static void processCommand(char *input, char *output);
static void getUptime(char *buff);

static pthread_t networkId;

int Server_configNetwork()
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

    pthread_create(&networkId, NULL, &Server_thread, NULL);

    return 0;
}

void Server_shutDownNetwork()
{
    close(socketDescriptor);
}

void *Server_thread()
{
    while (true)
    {
        char messageRx[MSG_MAX_LEN];
        memset(messageRx, '\0', sizeof(messageRx));
        socklen_t sin_len = sizeof(client);
        int bytesRx = recvfrom(socketDescriptor,
                               messageRx, MSG_MAX_LEN, 0,
                               (struct sockaddr *)&client, &sin_len);

        // Make it null terminated
        int terminateIdx = (bytesRx < MSG_MAX_LEN) ? bytesRx : MSG_MAX_LEN - 1;
        messageRx[terminateIdx] = '\0';
        char outResponse[MSG_MAX_LEN];
        processCommand(messageRx, outResponse);

        Server_sendPacket(outResponse);
    }
}

void Server_sendPacket(char *messageTx)
{
    // Transmit a reply:
    unsigned int sin_len = sizeof(client);
    printf("Sending packet over UDP: %s\n", messageTx);
    sendto(socketDescriptor,
           messageTx, strlen(messageTx),
           0,
           (const struct sockaddr *)&client, sin_len);
}

static void processCommand(char *input, char *output)
{
    memset(output, '\0', MSG_MAX_LEN);
    if (strncmp(input, "volume_up", strlen(input)) == 0)
    {
        Helper_changeVolume("up");
        sprintf(output, "volume&%d", AudioMixer_getVolume());
    }
    else if (strncmp(input, "volume_down", strlen(input)) == 0)
    {
        Helper_changeVolume("down");
        sprintf(output, "volume&%d", AudioMixer_getVolume());
    }
    else if (strncmp(input, "bpm_up", strlen(input)) == 0)
    {
        Helper_changeTempo("up");
        sprintf(output, "bpm&%d", AudioMixer_getBPM());
    }
    else if (strncmp(input, "bpm_down", strlen(input)) == 0)
    {
        Helper_changeTempo("down");
        sprintf(output, "bpm&%d", AudioMixer_getBPM());
    }
    else if (strncmp(input, "bpm_get", strlen(input)) == 0)
    {
        sprintf(output, "bpm&%d", AudioMixer_getBPM());
    }
    else if (strncmp(input, "volume_get", strlen(input)) == 0)
    {
        sprintf(output, "volume&%d", AudioMixer_getVolume());
    }
    else if (strncmp(input, "hi-hat", strlen(input)) == 0)
    {
        sprintf(output, "hihat played");
        AudioMixer_queueSound(hihat);
    }
    else if (strncmp(input, "snare", strlen(input)) == 0)
    {
        sprintf(output, "snare played");
        AudioMixer_queueSound(snare);
    }
    else if (strncmp(input, "base", strlen(input)) == 0)
    {
        sprintf(output, "base played");
        AudioMixer_queueSound(base);
    }
    else if (strncmp(input, "mode1", strlen(input)) == 0)
    {
        sprintf(output, "%d", 1);
        pthread_mutex_lock(&Beatbox_playbackMutex);
        {
            Beatbox_currentlyPlayingSound = ROCK1;
        }
        pthread_mutex_unlock(&Beatbox_playbackMutex);
    }
    else if (strncmp(input, "mode2", strlen(input)) == 0)
    {
        sprintf(output, "%d", 2);
        pthread_mutex_lock(&Beatbox_playbackMutex);
        {
            Beatbox_currentlyPlayingSound = ROCK2;
        }
        pthread_mutex_unlock(&Beatbox_playbackMutex);
    }
    else if (strncmp(input, "none", strlen(input)) == 0)
    {
        sprintf(output, "%d", 0);
        pthread_mutex_lock(&Beatbox_playbackMutex);
        {
            Beatbox_currentlyPlayingSound = NONE;
        }
        pthread_mutex_unlock(&Beatbox_playbackMutex);
    }
    else if (strncmp(input, "uptime", strlen(input)) == 0)
    {
        char uptime[100];
        getUptime(uptime);
        sprintf(output, "uptime&%s", uptime);
    }
}

static void getUptime(char *buff)
{

    FILE *pFile = fopen(UPTIME, "r");
    if (pFile == NULL)
    {
        fprintf(stderr, "Unable to open path for writing\n");
        exit(-1);
    }
    fgets(buff, 100, pFile);
    fclose(pFile);
}
