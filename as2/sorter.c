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
#include "handle14SegDisplay.h"
#include "pothead.h"
#include "networking.h"

#define DEFAULT 100
#define MAX_BUFFER_SIZE 50000
#define MAX_UDP_PACKET_SIZE 1500
#define numArr 5
#define NUMBER_SIZE 6
#define DIGIT_SIZE 2

// initialize mutexes and conditionals
pthread_mutex_t runningMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t arrMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t littyMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t dontRead = PTHREAD_COND_INITIALIZER;
pthread_cond_t dontDelete = PTHREAD_COND_INITIALIZER;

// Holds all information needed for array sorting
struct sortedAnswer
{
    int *arr;
    int arrSize;
    int nextSize;
    int sortedCount;
    bool keepSorting;
};

// thread ids
static pthread_t tid;
static pthread_t networkId;
static pthread_t potId;

static struct sortedAnswer answer;
static void swap(int *i, int *j);
static void Sorter_readUserInput(char *input, char *output);
void *bubbleSort(void *ans);
void *networkHandler();
void *potHandler();
static int getArrayElem(int idx);
static int parseDigit(char *input);
int i2cFileDesc;
long long numSortedLastSecond;
// Condition variable to prevent deleting an array then read the array
static bool arrInitialized;

// Terminating variable
static bool running;
static bool readArr = false;

int main(int argc, char *argv[])
{
    answer.nextSize = -1;
    answer.sortedCount = 0;
    answer.keepSorting = false;

    Networking_configNetwork();
    i2cFileDesc = configureI2C();

    // starts sorting thread
    Sorter_startSorting();

    bool mainRunning = true;
    while (mainRunning)
    {
        pthread_mutex_lock(&runningMutex);
        {
            pthread_mutex_lock(&littyMutex);
            {
                Seg_writeNumber(i2cFileDesc, numSortedLastSecond);
            }
            pthread_mutex_unlock(&littyMutex);
        }
        mainRunning = running;
        pthread_mutex_unlock(&runningMutex);
    }

    Sorter_stopSorting();
    Networking_shutDownNetwork();
    shutDownI2C(i2cFileDesc);

    return 0;
}

void *potHandler()
{
    bool potRunning = true;
    while (potRunning)
    {
        long long numSortedBefore = Sorter_getNumberArraysSorted();
        long seconds = 1;
        long nanoseconds = 0;
        struct timespec reqDelay = {seconds, nanoseconds};
        nanosleep(&reqDelay, (struct timespec *)NULL);
        long long numSortedAfter = Sorter_getNumberArraysSorted();
        pthread_mutex_lock(&littyMutex);
        {
            numSortedLastSecond = numSortedAfter - numSortedBefore;
        }
        pthread_mutex_unlock(&littyMutex);

        int a2d = Pothead_getVoltage0Reading();
        int size = Pothead_calcArraySize(a2d);
        printf("New array size: %d\n", size);

        Sorter_setArraySize(size);

        pthread_mutex_lock(&runningMutex);
        {
            potRunning = running;
        }
        pthread_mutex_unlock(&runningMutex);
    }
    pthread_exit(0);
}

void Sorter_startSorting(void)
{
    pthread_mutex_lock(&runningMutex);
    {
        running = true;
    }
    pthread_mutex_unlock(&runningMutex);
    pthread_create(&networkId, NULL, &networkHandler, NULL);
    pthread_create(&potId, NULL, &potHandler, NULL);
    pthread_create(&tid, NULL, &bubbleSort, &answer);
}

void Sorter_stopSorting(void)
{
    pthread_mutex_lock(&runningMutex);
    {
        running = false;
    }
    pthread_mutex_unlock(&runningMutex);
    pthread_join(tid, NULL);
    pthread_join(networkId, NULL);
    pthread_join(potId, NULL);
    pthread_mutex_destroy(&arrMutex);
    pthread_mutex_destroy(&runningMutex);
    pthread_mutex_destroy(&littyMutex);
    pthread_cond_destroy(&dontRead);
}

void *bubbleSort(void *ans)
{
    time_t t;
    srand((unsigned)time(&t));
    struct sortedAnswer *answer = ans;
    bool sortRunning = true;
    while (sortRunning)
    {
        int currentSize;
        // Initialize Array
        pthread_mutex_lock(&arrMutex);
        {
            if (answer->nextSize != -1)
            {
                answer->arrSize = answer->nextSize;
            }
            else
            {
                answer->arrSize = DEFAULT;
            }
            currentSize = answer->arrSize;
            // create new array
            answer->arr = malloc(currentSize * sizeof(int));
            // populate array with elements

            for (int i = 0; i < currentSize; i++)
            {
                answer->arr[i] = i + 1;
            }
            // shuffle elements of the array
            for (int i = 0; i < currentSize; i++)
            {
                int randIdx = rand() % currentSize;
                swap(&answer->arr[i], &answer->arr[randIdx]);
            }
            arrInitialized = true;
        }
        pthread_mutex_unlock(&arrMutex);
        pthread_cond_signal(&dontRead);

        // Sort array
        for (int i = 0; i < currentSize - 1; i++)
        {
            for (int j = 0; j < currentSize - i - 1; j++)
            {
                pthread_mutex_lock(&arrMutex);
                {
                    if (answer->arr[j] > answer->arr[j + 1])
                    {
                        swap(&answer->arr[j], &answer->arr[j + 1]);
                    }
                }
                pthread_mutex_unlock(&arrMutex);
            }
        }

        // check if anyone is reading current array
        // if not, then free
        pthread_mutex_lock(&arrMutex);
        {
            if (readArr)
            {
                pthread_cond_wait(&dontDelete, &arrMutex);
            }
            free(answer->arr);
            answer->arr = NULL;
            arrInitialized = false;
            answer->sortedCount++;
        }

        pthread_mutex_unlock(&arrMutex);

        pthread_mutex_lock(&runningMutex);
        {
            sortRunning = running;
        }
        pthread_mutex_unlock(&runningMutex);
    }
    pthread_exit(0);
}

static void swap(int *i, int *j)
{
    int temp = *i;
    *i = *j;
    *j = temp;
}

void Sorter_setArraySize(int newSize)
{
    pthread_mutex_lock(&arrMutex);
    {
        answer.nextSize = newSize;
    }
    pthread_mutex_unlock(&arrMutex);
}

int Sorter_getArrayLength(void)
{
    int size;
    pthread_mutex_lock(&arrMutex);
    {
        size = answer.arrSize;
    }
    pthread_mutex_unlock(&arrMutex);
    return size;
}

void Sorter_getArrayData(char *copy)
{
    char temp[NUMBER_SIZE]; // size 6 to include 4 digits, a comma and null termination character
    memset(copy, '\0', MAX_BUFFER_SIZE);

    pthread_mutex_lock(&arrMutex);
    {
        readArr = true;
        if (!arrInitialized)
        {
            pthread_cond_wait(&dontRead, &arrMutex);
        }

        for (int i = 1; i < answer.arrSize + 1; i++)
        {
            memset(temp, '\0', NUMBER_SIZE);
            if (i != answer.arrSize)
            {
                if (i % 10 != 0)
                {
                    sprintf(temp, "%d, ", answer.arr[i - 1]);
                }
                else
                {
                    sprintf(temp, "%d\n", answer.arr[i - 1]);
                }
            }
            else
            {
                sprintf(temp, "%d\n", answer.arr[i - 1]);
            }

            strcat(copy, temp);
        }
        readArr = false;
    }
    pthread_mutex_unlock(&arrMutex);
    pthread_cond_signal(&dontDelete);
}

long long Sorter_getNumberArraysSorted(void)
{
    return answer.sortedCount;
}

static void Sorter_readUserInput(char *input, char *output)
{
    memset(output, '\0', MSG_MAX_LEN);
    if (strncmp(input, "count", strlen("count")) == 0)
    {
        sprintf(output, "Number of arrays sorted = %lld\n", Sorter_getNumberArraysSorted());
    }
    else if (strncmp(input, "help", strlen("help")) == 0)
    {
        sprintf(output, "Accepted command examples:\n"
                        "count      -- display number arrays sorted.\n"
                        "get length -- display length of array currently being sorted.\n"
                        "get array  -- display the full array being sorted.\n"
                        "get 10     -- display the tenth element of array currently being sorted.\n"
                        "stop       -- cause the server program to end.\n");
    }
    else if (strncmp(input, "get array", strlen("get array")) == 0)
    {
        char char_arr[MAX_BUFFER_SIZE];
        Sorter_getArrayData(char_arr);
        sprintf(output, "%s", char_arr);
    }
    else if (strncmp(input, "get length", strlen("get length")) == 0)
    {
        sprintf(output, "Current array length = %d\n", Sorter_getArrayLength());
    }
    else if (strncmp(input, "stop", strlen("stop")) == 0)
    {
        running = false;
    }
    else if (strncmp(input, "get", strlen("get")) == 0)
    {
        int val = parseDigit(input);
        int len = Sorter_getArrayLength();

        if (val > len || val <= 0)
        {
            sprintf(output, "Invalid argument. Must be between 1 and %d (array length).\n", len);
        }
        else
        {
            sprintf(output, "Value = %d\n", getArrayElem(val - 1));
        }
    }
    else
    {
        sprintf(output, "Command not recognized. Try typing help to see a list of commands.\n");
    }
}

static int parseDigit(char *input)
{
    char nums[NUMBER_SIZE] = "";
    char temp[DIGIT_SIZE];
    int val = -1;
    for (int i = 4; i < strlen(input) - 1; i++)
    {
        if (isdigit(input[i]) != 0)
        {
            sprintf(temp, "%c", input[i]);
            strcat(nums, temp);
        }
        else
        {
            break;
        }
    }

    if (strlen(nums) != 0)
    {
        val = atoi(nums);
    }

    return val;
}

static int getArrayElem(int idx)
{
    int value = -1;
    pthread_mutex_lock(&arrMutex);
    {
        readArr = true;
        if (!arrInitialized)
        {
            pthread_cond_wait(&dontRead, &arrMutex);
        }
        value = answer.arr[idx];
        readArr = false;
    }
    pthread_mutex_unlock(&arrMutex);
    pthread_cond_signal(&dontDelete);

    return value;
}

// Function to split packets to send using UDP
void split_packets(char *outResponse, int outResponseSize)
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

void *networkHandler()
{
    //needs to continually send and recieve packets until thread is shutdow
    bool networkRunning = true;
    while (networkRunning)
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
        Sorter_readUserInput(messageRx, outResponse);

        int outResponseSize = strlen(outResponse) * sizeof(char);
        if (outResponseSize < MAX_UDP_PACKET_SIZE)
        {
            Networking_sendPacket(outResponse);
        }
        else
        {
            split_packets(outResponse, outResponseSize);
        }

        pthread_mutex_lock(&runningMutex);
        {
            networkRunning = running;
        }
        pthread_mutex_unlock(&runningMutex);
    }
    pthread_exit(0);
}