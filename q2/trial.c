#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <string.h>
#include <semaphore.h>
#include <error.h>
#include <errno.h>

int main()
{
    srand(time(NULL));
    double sum = 0;
    int size = 1000000;
    float prob = (float)rand() / (float)RAND_MAX;
    for (int i = 0; i < size; i++)
    {
        float prob = (float)rand() / (float)RAND_MAX;
        sum += prob;
    }
    printf("%lf", sum / size);
}