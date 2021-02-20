#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>
#include <pthread.h>

int main(int argc, char *argv[])
{

    // Structs to hold data
    // sharedData shared_Data;
    // NetworkInfo remoteInfo;
    // threadInfo thread_info;

    int my_port;
    int remote_port;
    char *remote_machine_name;

    if (argc == 4)
    {
        my_port = atoi(argv[1]);
        remote_machine_name = argv[2];
        remote_port = atoi(argv[3]);
    }
    else if (argc < 4)
    {
        printf("Arguments of format:\n");
        printf("[my port number] [remote machine name] [remote port number]\n");
        return 0;
    }
    else if (my_port <= 1024 && my_port >= 0)
    {
        printf("You are trying to open a reserved port!\n");
        return 0;
    }

    // getaddrinfo from remote
    /* Arguments from terminal:
        argv[1] - my_port
        argv[2] - remote machine name
        argv[3] - remote_port
    */

    // Service requested (from the other client)
    struct addrinfo hints, *res;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_PASSIVE;
    hints.ai_protocol = IPPROTO_UDP;
    hints.ai_canonname = NULL;
    hints.ai_addr = NULL;
    hints.ai_next = NULL;

    // get info like IP from the other user
    int getAddInfo = getaddrinfo(argv[2], argv[3], &hints, &res);
    if (getAddInfo != 0)
    {
        fprintf(stderr, "Error in getaddinfo: %s\n", gai_strerror(getAddInfo));
        return 0;
    }
    remoteInfo.res = res;

    int socketDescriptor;
    struct sockaddr_in sin;
    memset(&sin, 0, sizeof(sin));
    // Get info about this local socket
    sin.sin_family = AF_INET;
    sin.sin_addr.s_addr = htonl(INADDR_ANY);
    sin.sin_port = htons(my_port);

    // Create socket for UDP
    socketDescriptor = socket(AF_INET, SOCK_DGRAM, 0); // create a socket and return a status response
    remoteInfo.local_socketDescriptor = socketDescriptor;

    if (socketDescriptor < 0)
    {
        printf("Error in creating UDP socket\n");
        return 0;
    }

    // Bind socket to specified port - Here we bind the socket to an IP and Port
    bind(socketDescriptor, (struct sockaddr *)&sin, sizeof(sin));

    // Initialize variables
    shared_Data.sendList = List_create();
    shared_Data.receiveList = List_create();
    shared_Data.terminationFlag = 0;
    shared_Data.toSendItems = 0;
    shared_Data.toReadItems = 0;
    pthread_mutex_init(&shared_Data.send_mutex, NULL);
    pthread_mutex_init(&shared_Data.receive_mutex, NULL);
    pthread_cond_init(&shared_Data.sendAvail, NULL);
    pthread_cond_init(&shared_Data.readAvail, NULL);

    printf("Starting chat...\n");

    _Input_init(&shared_Data, &thread_info);
    _Sender_init(&shared_Data, &remoteInfo, &thread_info);
    _Receiver_init(&shared_Data, &remoteInfo, &thread_info);
    _Screen_init(&shared_Data, &thread_info);

    _Input_shutdown();
    _Sender_shutdown();
    _Receiver_shutdown();
    _Screen_shutdown();

    // Cleaning up variables
    pthread_mutex_destroy(&shared_Data.send_mutex);
    pthread_mutex_destroy(&shared_Data.receive_mutex);
    pthread_cond_destroy(&shared_Data.sendAvail);
    pthread_cond_destroy(&shared_Data.readAvail);
    List_free(shared_Data.sendList, freeLists);
    List_free(shared_Data.receiveList, freeLists);

    freeaddrinfo(res);
    close(socketDescriptor);

    printf("Terminating program...\n");

    return 0;
}