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
pthread_mutex_t reading_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t network = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t arrMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t arrSizeMutex = PTHREAD_MUTEX_INITIALIZER;

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

pthread_t tid;
static struct sortedAnswer answer;
static void swap(int *i, int *j);
static void Sorter_readUserInput(char *input, char *output);
static bool networkRunning;
static pthread_t networkId;
void *bubbleSort(void *ans);
void *networkHandler();
static int getArrayElem(int idx);
static int parseDigit(char *input);

int main(int argc, char *argv[])
{
    answer.nextSize = -1;
    answer.sortedCount = 0;
    answer.keepSorting = false;
    networkRunning = true;

    Networking_configNetwork();
    configureI2C(i2cFileDesc);
    pthread_create(&networkId, NULL, &networkHandler, NULL);
    // starts sorting thread
    Sorter_startSorting();

    // create while loop that keeps going until stop command is reached
    // need to trigger some global variable or mutex to signal that we need to break loop

    networkHandler();
    printf("hello, network is initialized");
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
    pthread_mutex_lock(&arrMutex);
    {
        answer.keepSorting = true;
    }
    pthread_mutex_unlock(&arrMutex);
    pthread_create(&tid, NULL, &bubbleSort, &answer);
}

void Sorter_stopSorting(void)
{
    pthread_mutex_lock(&arrMutex);
    {
        answer.keepSorting = false;
    }
    pthread_mutex_unlock(&arrMutex);
    pthread_join(tid, NULL);
    free(answer.arr);
    answer.arr = NULL;
}

void *bubbleSort(void *ans)
{
    struct sortedAnswer *answer = ans;
    while (true)
    {

        pthread_mutex_lock(&arrSizeMutex);
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
        pthread_mutex_unlock(&arrSizeMutex);

        // create new array
        answer->arr = malloc(answer->arrSize * sizeof(int));

        time_t t;
        srand((unsigned)time(&t));
        // populate array with elements
        for (int i = 0; i < answer->arrSize; i++)
        {
            answer->arr[i] = i;
        }

        // shuffle elements of the array
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
                    pthread_mutex_lock(&arrMutex);
                    {
                        swap(&answer->arr[j], &answer->arr[j + 1]);
                        if (answer->keepSorting == false)
                        {
                            pthread_mutex_unlock(&arrMutex);
                            pthread_exit(0);
                        }
                    }
                    pthread_mutex_unlock(&arrMutex);
                }
            }
        }

        answer->sortedCount++;

        // check if anyone is reading current array
        // if not, then free
        pthread_mutex_lock(&reading_lock);
        {
            free(answer->arr);
            answer->arr = NULL;
        }
        pthread_mutex_unlock(&reading_lock);
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
    pthread_mutex_lock(&arrSizeMutex);
    {
        answer.nextSize = newSize;
    }
    pthread_mutex_unlock(&arrSizeMutex);
}

int Sorter_getArrayLength(void)
{
    int size;
    pthread_mutex_lock(&arrSizeMutex);
    {
        size = answer.arrSize;
    }
    pthread_mutex_unlock(&arrSizeMutex);
    return size;
}

// Get a copy of the current (potentially partially sorted) array.
// Returns a newly allocated array and sets 'length' to be the
// number of elements in the returned array (output-only parameter).
// The calling code must call free() on the returned pointer.
char *Sorter_getArrayData(int *length)
{
    char *copy;
    char temp[DIGIT_SIZE]; // size 6 to include 4 digits, a comma and null termination character

    pthread_mutex_lock(&arrSizeMutex);
    {
        copy = malloc(sizeof(int) * answer.arrSize);
        *length = answer.arrSize;
    }
    pthread_mutex_unlock(&arrSizeMutex);

    pthread_mutex_lock(&arrMutex);
    {
        int j;
        for (int i = 0; i < *length; i += 10)
        {
            for (j = 0; j < 9; j++)
            {
                memset(temp, '\0', DIGIT_SIZE);
                sprintf(temp, "%c, ", answer.arr[i + j]);
            }
            sprintf(temp, "%c\n", answer.arr[i + j + 1]);
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
        int *len = NULL;
        sprintf(output, "%s", Sorter_getArrayData(len));
    }
    else if (strncmp(input, "get length", strlen("get length")) == 0)
    {
        sprintf(output, "Current array length = %d\n", Sorter_getArrayLength());
    }
    else if (strncmp(input, "stop", strlen("stop")) == 0)
    {
        networkRunning = false;
        Sorter_stopSorting();
    }
    else if (strncmp(input, "get", strlen("get")) == 0)
    {
        int val = parseDigit(input);
        int len = Sorter_getArrayLength();

        if (val > len || val < 0)
        {
            sprintf(output, "Invalid argument. Must be between 0 and %d (array length).\n", len);
        }
        else
        {
            sprintf(output, "Value = %d\n", getArrayElem(val));
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
        pthread_mutex_lock(&network);
        {
            running = networkRunning;
        }
        pthread_mutex_unlock(&network);
    }
    return NULL;
}