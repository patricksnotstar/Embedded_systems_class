#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdbool.h>
#include <pthread.h>
#include <string.h>
#include <ctype.h>
#include <netdb.h>
#include <unistd.h> // for close()

#include "sorter.h"
#include "littyDaSeggy.h"
#include "pothead.h"
#include "networking.h"

#define DEFAULT 100
#define numArr 5

struct sortedAnswer
{
    pthread_t tid;
    int *arr;
    int arrSize;
    int nextSize;
    pthread_mutex_t arrMutex;
    pthread_mutex_t arrSizeMutex;
    int sortedCount;
    bool keepSorting;
};

static struct sortedAnswer answer;
static void swap(int *i, int *j);
static int Sorter_readUserInput(char *input, char *output);
static bool networkRunning;
pthread_mutex_t network;
static pthread_t networkId;
void *bubbleSort(void *ans);
void *networkHandler(void);

int main(int argc, char *argv[])
{

    if (pthread_mutex_init(&answer.arrMutex, NULL) != 0)
    {
        printf("Mutex initialization failed\n");
        return 1;
    }

    if (pthread_mutex_init(&answer.arrSizeMutex, NULL) != 0)
    {
        printf("Mutex initialization failed\n");
        return 1;
    }

    if (pthread_mutex_init(&network, NULL) != 0)
    {
        printf("Mutex initialization failed\n");
        return 1;
    }
    answer.nextSize = -1;
    answer.sortedCount = 0;
    answer.keepSorting = false;

    Networking_configNetwork();
    configureI2C(i2cFileDesc);
    pthread_create(&networkId, NULL, *networkHandler, NULL);
    // starts sorting thread
    Sorter_startSorting();

    // create while loop that keeps going until stop command is reached
    // need to trigger some global variable or mutex to signal that we need to break loop

    // trigger shutdown operations after while loop is done

    Sorter_stopSorting();
    Networking_shutDownNetwork();
    shutDownI2C(i2cFileDesc);

    // // repeatedly call bubble sort on random arrays, with sizes read from POT
    // // stop sorting once UDP socket gets stop command

    // while (true)
    // {
    //     long seconds = 1;
    //     long nanoseconds = 0;
    //     struct timespec reqDelay = {seconds, nanoseconds};
    //     nanosleep(&reqDelay, (struct timespec *)NULL);

    //     int a2d = Pothead_getVoltage0Reading();

    //     int size = Pothead_calcArraySize(a2d);
    //     Sorter_setArraySize(size);
    // }

    return 0;
}

void Sorter_startSorting(void)
{
    pthread_t tid;
    pthread_mutex_lock(&answer.arrMutex);
    {
        answer.keepSorting = true;
    }
    pthread_mutex_unlock(&answer.arrMutex);
    pthread_create(&tid, NULL, &bubbleSort, &answer);
    answer.tid = tid;
}

void Sorter_stopSorting(void)
{
    pthread_mutex_lock(&answer.arrMutex);
    {
        answer.keepSorting = false;
    }
    pthread_mutex_unlock(&answer.arrMutex);
    pthread_join(answer.tid, NULL);
    free(answer.arr);
    answer.arr = NULL;
}

void *bubbleSort(void *ans)
{
    struct sortedAnswer *answer = ans;
    while (true)
    {

        pthread_mutex_lock(&answer->arrSizeMutex);
        {
            if (answer->nextSize != -1)
            {
                answer->arrSize = answer->nextSize;
            }
            else
            {
                answer->arrSize = DEFAULT;
            }
        }
        pthread_mutex_unlock(&answer->arrSizeMutex);

        answer->arr = malloc(answer->arrSize * sizeof(int));

        time_t t;
        srand((unsigned)time(&t));
        for (int i = 0; i < answer->arrSize; i++)
        {
            answer->arr[i] = i;
        }

        for (int i = 0; i < answer->arrSize; i++)
        {
            int randIdx = rand() % answer->arrSize;
            swap(&answer->arr[i], &answer->arr[randIdx]);
        }

        for (int i = 0; i < answer->arrSize - 1; i++)
        {
            for (int j = 0; j < answer->arrSize - i - 1; j++)
            {
                if (answer->arr[j] > answer->arr[j + 1])
                {
                    pthread_mutex_lock(&answer->arrMutex);
                    {
                        swap(&answer->arr[j], &answer->arr[j + 1]);
                        if (answer->keepSorting == false)
                        {
                            pthread_mutex_unlock(&answer->arrMutex);
                            pthread_exit(0);
                        }
                    }
                    pthread_mutex_unlock(&answer->arrMutex);
                }
            }
        }

        answer->sortedCount++;
    }
}

static void swap(int *i, int *j)
{
    int temp = *i;
    *i = *j;
    *j = temp;
}

void Sorter_setArraySize(int newSize)
{
    pthread_mutex_lock(&answer.arrSizeMutex);
    {
        answer.nextSize = newSize;
    }
    pthread_mutex_unlock(&answer.arrSizeMutex);
}

int Sorter_getArrayLength(void)
{
    int size;
    pthread_mutex_lock(&answer.arrSizeMutex);
    {
        size = answer.arrSize;
    }
    pthread_mutex_unlock(&answer.arrSizeMutex);
    return size;
}

// Get a copy of the current (potentially partially sorted) array.
// Returns a newly allocated array and sets 'length' to be the
// number of elements in the returned array (output-only parameter).
// The calling code must call free() on the returned pointer.
char *Sorter_getArrayData(int *length)
{
    char *copy;

    pthread_mutex_lock(&answer.arrSizeMutex);
    {
        copy = malloc(sizeof(int) * answer.arrSize);
        *length = answer.arrSize;
    }
    pthread_mutex_unlock(&answer.arrSizeMutex);

    pthread_mutex_lock(&answer.arrMutex);
    {
        for (int i = 0; i < *length; i++)
        {
            copy[i] = answer.arr[i];
        }
    }
    pthread_mutex_unlock(&answer.arrMutex);

    return copy;
}

long long Sorter_getNumberArraysSorted(void)
{
    return answer.sortedCount;
}

static int Sorter_readUserInput(char *input, char *output)
{
    if (strncmp(input, "count", strlen("count") == 0))
    {
        sprintf(output, "Number of arrays sorted = %lld\n", Sorter_getNumberArraysSorted());
        return 0;
    }
    else if (strncmp(input, "help", strlen("help")) == 0)
    {
        output = "Accepted command examples:\n"
                 "count      -- display number arrays sorted.\n"
                 "get length -- display length of array currently being sorted.\n"
                 "get array  -- display the full array being sorted.\n"
                 "get 10     -- display the tenth element of array currently being sorted.\n"
                 "stop       -- cause the server program to end.\n";
        return 0;
    }
    else if (strncmp(input, "get array", strlen("get array") == 0))
    {
        int *len;
        char *arr = Sorter_getArrayData(len);
        int j;
        for (int i = 0; i < *len; i += 10)
        {
            for (j = 0; j < 9; j++)
            {
                sprintf(output, "%c, ", arr[i + j]);
            }

            sprintf(output, "%c\n", arr[i + j + 1]);
        }
        return 0;
    }
    else if (strncmp(input, "get length", strlen("get length") == 0))
    {
        sprintf(output, "Current array length = %d\n", Sorter_getArrayLength());
        return 0;
    }
    else if (strncmp(input, "stop", strlen("stop") == 0))
    {
        Sorter_stopSorting();
        return 0;
    }
    else if (strncmp(input, "get", strlen("get")) == 0)
    {
        char *nums[5];
        int j = 0;
        for (int i = 3; i < strlen(input) - 1; i++)
        {
            if (isdigit(input[i]))
            {
                *nums[j++] = input[i];
            }
            else
            {
                output = "Command not recognized. Try typing help to see a list of commands.\n";
                break;
            }
        }
        int val = atoi(*nums);

        int *len;
        char *arr = Sorter_getArrayData(len);
        if (val > *len || val < 0)
        {
            output = "Value is out of bounds for the current array being sorted. Try again.\n";
        }
        else
        {
            sprintf(output, "Value = %d\n", arr[val]);
        }
        return 0;
    }
    else
    {
        output = "Command not recognized. Try typing help to see a list of commands.\n";
    }

    return -1;
}

void *networkHandler(void)
{ //needs to continually send and recieve packets until thread is shutdown
    bool running;
    pthread_mutex_lock(&network);
    {
        running = networkRunning;
    }
    pthread_mutex_unlock(&network);

    while (running)
    {
        char messageRx[MSG_MAX_LEN];
        memset(messageRx, 0, sizeof(messageRx));
        unsigned int sin_len = sizeof(client);
        int bytesRx = recvfrom(socketDescriptor,
                               messageRx, MSG_MAX_LEN, 0,
                               (struct sockaddr *)&client, &sin_len);

        // Make it null terminated
        int terminateIdx = (bytesRx < MSG_MAX_LEN) ? bytesRx : MSG_MAX_LEN - 1;
        messageRx[terminateIdx] = 0;

        char outResponse[MSG_MAX_LEN];
        Sorter_readUserInput(messageRx, outResponse);

        Networking_sendPacket(outResponse);
        pthread_mutex_lock(&network);
        {
            running = networkRunning;
        }
        pthread_mutex_unlock(&network);
    }
}