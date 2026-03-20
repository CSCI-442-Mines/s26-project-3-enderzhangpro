#define _XOPEN_SOURCE 600
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "pzip.h"
#include <bits/pthreadtypes.h>

/**
 * pzip() - zip an array of characters in parallel
 *
 * Inputs:
 * @n_threads:		   The number of threads to use in pzip
 * @input_chars:		   The input characters (a-z) to be zipped
 * @input_chars_size:	   The number of characaters in the input file
 *
 * Outputs:
 * @zipped_chars:       The array of zipped_char structs
 * @zipped_chars_count:   The total count of inserted elements into the zipped_chars array
 * @char_frequency: Total number of occurences
 *
 * NOTE: All outputs are already allocated. DO NOT MALLOC or REASSIGN THEM !!!
 *
 */

pthread_barrier_t barrier;
pthread_mutex_t *lock;
int increBy;
int lock_index = 0;
int zipped_chars_index = 0;

struct Args
{
    char *input_chars;
    int input_chars_size;
    int start;
    struct zipped_char *zipped_chars;
    int *char_frequency;
};

static void *helper_function(void* a)
{
  struct Args *args = (struct Args*) a;
  char previous_c = 'A';
  struct zipped_char *temp_chars = malloc(sizeof(struct zipped_char) * increBy);
  int temp_chars_index = 0;
  // fprintf(stderr, "Start\n");
  // fprintf(stderr, "%d\n", args->start);
  // fprintf(stderr, "%d\n", args->start + increBy);
  for (int i = args->start; i < args->start + increBy; i++)
  {
    char current_c = args->input_chars[i];
    // fprintf(stderr, "%c", current_c);
    if (previous_c == current_c)
    {
      temp_chars[temp_chars_index - 1].occurence++;
    }
    else
    {
      previous_c = current_c;
      temp_chars[temp_chars_index].character = current_c;
      temp_chars[temp_chars_index].occurence = 1;
      temp_chars_index++;
    }
  }
  // fprintf(stderr, "\n");
  pthread_barrier_wait(&barrier);

  pthread_mutex_unlock(&lock[lock_index]);
  pthread_mutex_lock(&lock[args->start / increBy]); // causing all threads to wait
  for (int i = 0; i < temp_chars_index; i++)
  {
    args->zipped_chars[zipped_chars_index] = temp_chars[i];
    args->char_frequency[args->zipped_chars[zipped_chars_index].character - 'a'] += args->zipped_chars[zipped_chars_index].occurence;
    // fprintf(stderr, "%i\n", args->zipped_chars[zipped_chars_index].occurence);
    zipped_chars_index++;
  }
  lock_index++;
  pthread_mutex_unlock(&lock[lock_index]);
  free(temp_chars);
  // fprintf(stderr, "Over");
  pthread_exit(NULL);
}

void pzip(int n_threads, char *input_chars, int input_chars_size,
	  struct zipped_char *zipped_chars, int *zipped_chars_count,
	  int *char_frequency)
{
	pthread_t thread_ids[n_threads];
  pthread_barrier_init(&barrier, NULL, n_threads);
  lock = (pthread_mutex_t*) malloc(sizeof(pthread_mutex_t) * n_threads);
  increBy = input_chars_size / n_threads;
  // fprintf(stderr, "%d\n\n", increBy);
  struct Args* argsArr = malloc(sizeof(struct Args) * n_threads);
  for (int i = 0; i < n_threads; i++)
  {
    argsArr[i].input_chars = input_chars;
    argsArr[i].input_chars_size = input_chars_size;
    argsArr[i].start = i * increBy;
    argsArr[i].zipped_chars = zipped_chars;
    argsArr[i].char_frequency = char_frequency;
    pthread_mutex_init(&lock[i], NULL);
    pthread_mutex_lock(&lock[i]);
    pthread_create(&thread_ids[i], NULL, helper_function, (void*) &argsArr[i]);
  }
  for (int i = 0; i < n_threads; i++)
  {
    void *result;
    pthread_join(thread_ids[i], &result);
  }
  free(lock);
  // free(argsArr);
  pthread_barrier_destroy(&barrier);
  *zipped_chars_count = zipped_chars_index;
}
