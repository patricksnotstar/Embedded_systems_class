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
#define DIGIT_SIZE 6

// initialize mutexes and conditionals
// pthread_mutex_t reading_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t network = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t arrMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t dontRead = PTHREAD_COND_INITIALIZER;
// pthread_mutex_t arrSizeMutex = PTHREAD_MUTEX_INITIALIZER;

// termination conditions
bool readVar = false;

struct sortedAnswer
{
    int *arr;
    int arrSize;
    int nextSize;
    int sortedCount;
    bool keepSorting;
};

static pthread_t tid;
static pthread_t networkId;
static pthread_t potId;
static struct sortedAnswer answer;
static void swap(int *i, int *j);
static void Sorter_readUserInput(char *input, char *output);
static bool running;
void *bubbleSort(void *ans);
void *networkHandler();
void *potHandler();
static int getArrayElem(int idx);
static int parseDigit(char *input);
static bool arrInitialized;

int main(int argc, char *argv[])
{
    answer.nextSize = -1;
    answer.sortedCount = 0;
    answer.keepSorting = false;

    Networking_configNetwork();
    configureI2C(i2cFileDesc);

    // starts sorting thread
    Sorter_startSorting();

    while (running)
    {
    }

    Sorter_stopSorting();
    Networking_shutDownNetwork();
    shutDownI2C(i2cFileDesc);

    return 0;
}

void *potHandler()
{
    while (true)
    {
        long long numSortedBefore = Sorter_getNumberArraysSorted();
        long seconds = 1;
        long nanoseconds = 0;
        struct timespec reqDelay = {seconds, nanoseconds};
        nanosleep(&reqDelay, (struct timespec *)NULL);

        int a2d = Pothead_getVoltage0Reading();
        int size = Pothead_calcArraySize(a2d);
        // printf("New array size: %d\n", size);

        Sorter_setArraySize(size);

        long long numSortedAfter = Sorter_getNumberArraysSorted();

        long long numSortedLastSecond = numSortedAfter - numSortedBefore;

        // printf("Number of arrays sorted last second: %lld\n", numSortedLastSecond);

        Seg_writeNumber(i2cFileDesc, numSortedLastSecond);

        bool potRunning;
        pthread_mutex_lock(&network);
        {
            potRunning = running;
        }
        pthread_mutex_unlock(&network);

        if (!potRunning)
        {
            pthread_exit(0);
        }
    }

    return 0;
}

void Sorter_startSorting(void)
{
    pthread_mutex_lock(&network);
    {
        running = true;
    }
    pthread_mutex_unlock(&network);
    pthread_create(&networkId, NULL, &networkHandler, NULL);
    pthread_create(&potId, NULL, &potHandler, NULL);
    pthread_create(&tid, NULL, &bubbleSort, &answer);
}

void Sorter_stopSorting(void)
{
    pthread_mutex_lock(&network);
    {
        running = false;
    }
    pthread_mutex_unlock(&network);
    pthread_join(tid, NULL);
    pthread_join(networkId, NULL);
    pthread_join(potId, NULL);
    free(answer.arr);
    answer.arr = NULL;
}

void *bubbleSort(void *ans)
{
    time_t t;
    srand((unsigned)time(&t));
    struct sortedAnswer *answer = ans;
    while (true)
    {
        int currentSize;
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

        for (int i = 0; i < currentSize - 1; i++)
        {
            for (int j = 0; j < currentSize - i - 1; j++)
            {
                pthread_mutex_lock(&arrMutex);
                {
                    if (answer->arr[j] > answer->arr[j + 1])
                    {
                        swap(&answer->arr[j], &answer->arr[j + 1]);
                        if (running == false)
                        {
                            pthread_mutex_unlock(&arrMutex);
                            pthread_exit(0);
                        }
                    }
                }
                pthread_mutex_unlock(&arrMutex);
            }
        }

        answer->sortedCount++;

        // check if anyone is reading current array
        // if not, then free
        pthread_mutex_lock(&arrMutex);
        {
            free(answer->arr);
            answer->arr = NULL;
            arrInitialized = false;
        }
        pthread_mutex_unlock(&arrMutex);
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

char *Sorter_getArrayData()
{
    char *copy;
    char temp[DIGIT_SIZE]; // size 6 to include 4 digits, a comma and null termination character

    pthread_mutex_lock(&arrMutex);
    {
        if (!arrInitialized)
        {
            pthread_cond_wait(&dontRead, &arrMutex);
        }
        copy = malloc(sizeof(int) * answer.arrSize);

        for (int i = 1; i < answer.arrSize + 1; i++)
        {
            memset(temp, '\0', DIGIT_SIZE);
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
    }
    pthread_mutex_unlock(&arrMutex);

    return copy;
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
        sprintf(output, "%s", Sorter_getArrayData());
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
    char nums[6] = "";
    char temp[2];
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
        value = answer.arr[idx];
    }
    pthread_mutex_unlock(&arrMutex);

    return value;
}

void *networkHandler()
{ //needs to continually send and recieve packets until thread is shutdow

    while (true)
    {
        char messageRx[MSG_MAX_LEN];
        memset(messageRx, '\0', sizeof(messageRx));
        unsigned int sin_len = sizeof(client);
        int bytesRx = recvfrom(socketDescriptor,
                               messageRx, MSG_MAX_LEN, 0,
                               (struct sockaddr *)&client, &sin_len);

        // Make it null terminated
        int terminateIdx = (bytesRx < MSG_MAX_LEN) ? bytesRx : MSG_MAX_LEN - 1;
        messageRx[terminateIdx] = '\0';
        char outResponse[MSG_MAX_LEN];
        Sorter_readUserInput(messageRx, outResponse);

        Networking_sendPacket(outResponse);
        bool networkRunning;
        pthread_mutex_lock(&network);
        {
            networkRunning = running;
        }
        pthread_mutex_unlock(&network);

        if (!networkRunning)
        {
            pthread_exit(0);
        }
    }
    return NULL;
}