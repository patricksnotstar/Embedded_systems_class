#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdbool.h>
#include <pthread.h>

#include "sorter.h"

#define DEFAULT 100
#define numArr 5

static struct sortedAnswer
{
    int *arr;
};

void *bubbleSort(void *arr, int n);

int main(int argc, char *argv[])
{
    int *arr = malloc(DEFAULT * sizeof(int));
    time_t t;
    srand((unsigned)time(&t));
    for (int i = 0; i < DEFAULT; i++)
    {
        arr[i] = rand() % DEFAULT;
    }

    struct sortedAnswer ans[numArr];

    bubbleSort(arr, DEFAULT);
    free(arr);
    arr = NULL;
    return 0;
}

void *bubbleSort(void *arr, int n)
{
    int *array = arr;
    for (int i = 0; i < n - 1; i++)
    {
        for (int j = 0; j < n - i - 1; j++)
        {
            if (array[j] > array[j + 1])
            {
                int temp = array[j];
                array[j] = array[j + 1];
                array[j + 1] = temp;
            }
        }
    }
    return 0;
}