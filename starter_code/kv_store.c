#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>

#include "common.h"
#include "ring_buffer.h"

typedef struct
{
    key_type key;
    value_type value;
    int is_occupied;
    pthread_mutex_t lock;
} HashEntry;

typedef struct
{
    HashEntry *entries;
    int size;
} HashTable;

HashTable hashTable;
pthread_t threads[200];

struct ring *ringBuffer;
int isRunning = 1;

void initialize_hashTable(int size)
{
    hashTable.size = size;
    hashTable.entries = malloc(size * sizeof(HashEntry));

    for (int i = 0; i < size; i++)
    {
        hashTable.entries[i].is_occupied = 0;
        pthread_mutex_init(&hashTable.entries[i].lock, NULL);
    }
}

void put(key_type k, value_type v)
{
    int index = hash_function(k, hashTable.size);
    int start = index;

    do
    {
        pthread_mutex_lock(&hashTable.entries[index].lock);
        if (!hashTable.entries[index].is_occupied || hashTable.entries[index].key == k)
        {
            hashTable.entries[index].key = k;
            hashTable.entries[index].value = v;
            hashTable.entries[index].is_occupied = 1;
            pthread_mutex_unlock(&hashTable.entries[index].lock);
            break;
        }
        pthread_mutex_unlock(&hashTable.entries[index].lock);
        index = (index + 1) % hashTable.size;
    } while (index != start);
}

value_type get(key_type k)
{
    int index = hash_function(k, hashTable.size);
    int start = index;
    value_type v = 0;

    do
    {
        pthread_mutex_lock(&hashTable.entries[index].lock);
        if (hashTable.entries[index].is_occupied && hashTable.entries[index].key == k)
        {
            v = hashTable.entries[index].value;
            pthread_mutex_unlock(&hashTable.entries[index].lock);
            break;
        }
        pthread_mutex_unlock(&hashTable.entries[index].lock);
        index = (index + 1) % hashTable.size;
    } while (index != start && hashTable.entries[index].is_occupied);

    return v;
}

void *server_thread(void *arg)
{
    struct buffer_descriptor bd;
    char *shared_mem_start = (char *)ringBuffer;

    while (isRunning)
    {
        ring_get(ringBuffer, &bd);
        if (bd.req_type == PUT)
        {
            put(bd.k, bd.v);
        }
        else if (bd.req_type == GET)
        {
            bd.v = get(bd.k);
        }

        struct buffer_descriptor *result = (struct buffer_descriptor *)(shared_mem_start + bd.res_off);
        memcpy(result, &bd, sizeof(struct buffer_descriptor));
        result->ready = 1;
    }
    return NULL;
}

int main(int argc, char *argv[])
{
    int num_threads = 0;
    int table_size = 200;
    char *shm_file = "shmem_file";
    int fd = open(shm_file, O_RDWR);
    struct stat file_stat;

    if (fd < 0)
    {
        printf("ERROR: Cannot open shared memory.\n");
        return EXIT_FAILURE;
    }
    if (fstat(fd, &file_stat) == -1)
    {
        perror("ERROR: Cannot get stats\n");
        return EXIT_FAILURE;
    }
    void *shared = mmap(NULL, file_stat.st_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    close(fd);

    if (shared == MAP_FAILED)
    {
        perror("Failed to map shared memory");
        return EXIT_FAILURE;
    }

    ringBuffer = (struct ring *)shared;
    for (int i = 1; i < argc; i++)
    {
        if (strcmp(argv[i], "-n") == 0 && i + 1 < argc)
        {
            num_threads = atoi(argv[++i]);
        }
        else if (strcmp(argv[i], "-s") == 0 && i + 1 < argc)
        {
            table_size = atoi(argv[++i]);
        }
        else
        {
            printf("Incorrect usage.\n");
            exit(EXIT_FAILURE);
        }
    }

    if (num_threads <= 0 || table_size <= 0)
    {
        printf("ERROR: values are negative or not all values completed\n");
        exit(EXIT_FAILURE);
    }

    initialize_hashTable(table_size);

    for (int i = 0; i < num_threads; i++)
    {
        if (pthread_create(&threads[i], NULL, server_thread, NULL) != 0)
        {
            perror("Failed to create thread");
            return EXIT_FAILURE;
        }
    }

    for (int i = 0; i < num_threads; i++)
    {
        pthread_join(threads[i], NULL);
    }

    return 0;
}