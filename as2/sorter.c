#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdbool.h>
#include <pthread.h>

#include "sorter.h"
#include "pothead.h"

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

void *bubbleSort(void *ans);

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
    answer.nextSize = -1;
    answer.sortedCount = 0;
    answer.keepSorting = false;

    // starts sorting thread
    Sorter_startSorting();

    // // repeatedly call bubble sort on random arrays, with sizes read from POT
    // // stop sorting once UDP socket gets stop command

    while (true)
    {
        long seconds = 1;
        long nanoseconds = 0;
        struct timespec reqDelay = {seconds, nanoseconds};
        nanosleep(&reqDelay, (struct timespec *)NULL);

        int a2d = Pothead_getVoltage0Reading();

        int size = Pothead_calcArraySize(a2d);
        Sorter_setArraySize(size);
    }

    Sorter_stopSorting();

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
            printf("Sorting array of size: %d\n", answer->arrSize);
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
int *Sorter_getArrayData(int *length)
{
    int *copy;
    copy = malloc(sizeof(int) * answer.arrSize);
    *length = answer.arrSize;

    pthread_mutex_lock(&answer.arrMutex);
    {
        for (int i = 0; i < answer.arrSize; i++)
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