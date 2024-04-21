#include <stdio.h>
#include "common.h"
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

typedef struct kv_node
{
    key_type key;
    value_type value;
    struct kv_node *next;
} kv_node;

typedef struct
{
    kv_node **buff;
    int size;
    pthread_mutex_t *locks; // Array of locks
} kv_store;

kv_store store;

void init_kv_store(int size)
{
    store.size = size;
    store.buff = malloc(sizeof(kv_node *) * store.size);
    store.locks = malloc(sizeof(pthread_mutex_t) * store.size);

    for (int i = 0; i < store.size; i++)
    {
        store.buff[i] = NULL;
        pthread_mutex_init(&store.locks[i], NULL);
    }
}
/**
 *  This function is used to insert a key-value pair into the store. If the key * already exists, it updates the associated value.
 */
void put(key_type k, value_type v)
{
    unsigned int index = hash_function(k, store.size);

    // Critical Zone Enter
    pthread_mutex_lock(&store.locks[index]);

    kv_node *node = store.buff[index];
    kv_node *prev = NULL;

    while (node)
    {
        if (node->key == k)
        {
            node->value = v;
            pthread_mutex_unlock(&store.locks[index]);
            return;
        }
        prev = node;
        node = node->next;
    }

    // If there is no key, create a new kv_node
    kv_node *new = malloc(sizeof(kv_node));
    new->key = k;
    new->value = v;
    new->next = NULL;

    if (prev)
    {
        prev->next = new;
    }
    else
    {
        store.buff[index] = new;
    }

    // Critical Zone Exit
    pthread_mutex_unlock(&store.locks[index]);
}

/**
 * This function is used to retrieve the value associated with a given key from * the store.
 *
 */
value_type get(key_type k)
{
    unsigned int index = hash_function(k, store.size);

    // Critical Zone Enter
    pthread_mutex_lock(&store.locks[index]);

    kv_node *node = store.buff[index];

    while (node)
    {
        if (node->key == k)
        {
            pthread_mutex_unlock(&store.locks[index]);
            return node->value;
        }
        node = node->next;
    }

    // Critical Zone Exit
    pthread_mutex_unlock(&store.locks[index]);

    // If the key is not found, it returns 0.
    return 0;
}

int main(int argc, char *argv[])
{
    int threads = -1;
    int hashtableSize = -1;

    for (int i = 1; i < argc; i++)
    {
        if (strcmp(argv[i], "-n") == 0 && i + 1 < argc)
        {
            threads = atoi(argv[++i]);
        }
        else if (strcmp(argv[i], "-s") == 0 && i + 1 < argc)
        {
            hashtableSize = atoi(argv[++i]);
        }
        else
        {
            printf("Incorrect usage.\n");
            exit(1);
        }
    }

    // Makes sure both values are filled.
    if (threads == -1 || hashtableSize == -1)
    {
        printf("Incorrect usage. Both values need to be filled\n");
    }

    init_kv_store(hashtableSize);

    return 0;
}