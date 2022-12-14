#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <wait.h>
#include <pthread.h>
#include "zemaphore.h"

#define NUM_THREADS 3
#define NUM_ITER 10

zem_t z1, z2, z3;

void *justprint(void *data)
{
  int thread_id = *((int *)data);

  for (int i = 0; i < NUM_ITER; i++)
  {
    if (thread_id == 0)
    {
      zem_down(&z3);
      printf("This is thread %d\n", thread_id);
      zem_up(&z1);
    }

    if (thread_id == 1)
    {
      zem_down(&z1);
      printf("This is thread %d\n", thread_id);
      zem_up(&z2);
    }

    if (thread_id == 2)
    {
      zem_down(&z2);
      printf("This is thread %d\n", thread_id);
      zem_up(&z3);
    }
  }
  return 0;
}

int main(int argc, char *argv[])
{
  pthread_t mythreads[NUM_THREADS];
  int mythread_id[NUM_THREADS];

  zem_init(&z1, 0);
  zem_init(&z2, 0);
  zem_init(&z3, 1);

  for (int i = 0; i < NUM_THREADS; i++)
  {
    mythread_id[i] = i;
    pthread_create(&mythreads[i], NULL, justprint, (void *)&mythread_id[i]);
  }

  for (int i = 0; i < NUM_THREADS; i++)
  {
    pthread_join(mythreads[i], NULL);
  }

  return 0;
}
