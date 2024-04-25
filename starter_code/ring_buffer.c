#include <stdio.h>

#include "ring_buffer.h"

// Indexing helpers
int next(uint32_t curr) {
    if (curr > (RING_SIZE - 1)) {
        printf("next warning: curr went out of bounds\n");
    }
    if (curr >= (RING_SIZE - 1)) {
        curr = 0;
    } else {
        curr++;
    }
    return curr;
}

int prev(uint32_t curr) {
    if (curr < 0) {
        printf("prev warning: curr went out of bounds\n");
    }
    if (curr <= 0) {
        curr = (RING_SIZE - 1);
    } else {
        curr--;
    }
    return curr;
}

/*
 * Initialize the ring
 * @param r A pointer to the ring
 * @return 0 on success, negative otherwise - this negative value will be
 * printed to output by the client program
*/
int init_ring(struct ring *r) {
    // Heads are incremented immediately
	r->c_tail = 0;
	r->c_head = 0;
    r->p_tail = 0;
	r->p_head = 0;
    for (int i = 0; i < RING_SIZE; i++) {
        r->buffer[i].k = 0;
        r->buffer[i].v = 0;
        r->buffer[i].ready = 0;
        r->buffer[i].req_type = 0;
        r->buffer[i].res_off = 0;
    }
    r->first_put = 1;

    if (pthread_mutex_init(&r->c_head_lock, NULL) != 0) { 
        printf("Could not initialize mutex lock\n"); 
        return -1;
    } 
    if (pthread_mutex_init(&r->p_head_lock, NULL) != 0) { 
        printf("Could not initialize mutex lock\n"); 
        return -1;
    } 
    if (pthread_mutex_init(&r->c_tail_lock, NULL) != 0) { 
        printf("Could not initialize mutex lock\n"); 
        return -1;
    } 
    if (pthread_mutex_init(&r->p_tail_lock, NULL) != 0) { 
        printf("Could not initialize mutex lock\n"); 
        return -1;
    } 

    return 0;
}

/*
 * Submit a new item - should be thread-safe
 * This call will block the calling thread if there's not enough space
 * @param r The shared ring
 * @param bd A pointer to a valid buffer_descriptor - This pointer is only
 * guaranteed to be valid during the invocation of the function
*/
void ring_submit(struct ring *r, struct buffer_descriptor *bd) {
    /**
     * "On both cores, ring->prod_head and ring->cons_tail are copied in 
     * local variables. The prod_next local variable points to the next 
     * element of the table, or several elements after in the case of bulk 
     * enqueue."
     * https://doc.dpdk.org/guides/prog_guide/ring_lib.html
    */
    
    if (r->first_put) {
        r->buffer[0].k = bd->k;
        r->buffer[0].v = bd->v;
        r->buffer[0].ready = bd->ready;
        r->buffer[0].req_type = bd->req_type;
        r->buffer[0].res_off = bd->res_off;
        r->first_put = 0;
        return;
    }

    printf("Put: Aquiring head lock...\n");
    pthread_mutex_lock(&r->p_head_lock);

    // Block on full
    while (next(r->p_head) == r->c_tail) {
        printf("Put: Stalling for space at %i for buffer with key %i value %i\n", 
            next(r->p_head), bd->k, bd->v);
    };
    
    r->p_head = next(r->p_head);
    uint32_t p_ind = r->p_head;

    pthread_mutex_unlock(&r->p_head_lock);
    printf("Put: Unlocked next head at %i for buffer with key %i value %i\n", 
        r->p_tail, bd->k, bd->v);

    // Deep copy bd to buffer at prod index
    r->buffer[p_ind].k = bd->k;
    r->buffer[p_ind].v = bd->v;
    r->buffer[p_ind].ready = bd->ready;
    r->buffer[p_ind].req_type = bd->req_type;
    r->buffer[p_ind].res_off = bd->res_off;

    // Block until tail
    while (next(r->p_tail) != p_ind) {
        printf("Put: Stalling for tail turn at %i for buffer with key %i value %i\n", 
        r->p_tail, bd->k, bd->v);
    }

    printf("Put: Aquiring tail lock at %i for buffer with key %i value %i\n", 
        r->p_tail, bd->k, bd->v);
    pthread_mutex_lock(&r->p_tail_lock);

    r->p_tail = next(r->p_tail);

    pthread_mutex_unlock(&r->p_tail_lock);
    printf("Put: Incremented tail to %i for buffer with key %i value %i\n", 
        r->p_tail, bd->k, bd->v);

}

/*
 * Get an item from the ring - should be thread-safe
 * This call will block the calling thread if the ring is empty
 * @param r A pointer to the shared ring 
 * @param bd pointer to a valid buffer_descriptor to copy the data to
 * Note: This function is not used in the clinet program, so you can change
 * the signature.
*/
void ring_get(struct ring *r, struct buffer_descriptor *bd) {

    printf("Get: Aquiring head lock for buffer with key %i value %i\n", bd->k, bd->v);
    pthread_mutex_lock(&r->c_head_lock);

    // Block on empty
    while (r->c_head == r->p_tail) {
        printf("Get: Stalling for new value at %i for buffer with key %i value %i\n", 
            next(r->c_head), bd->k, bd->v);
    };

    r->c_head = next(r->c_head);
    uint32_t c_ind = r->c_head;

    pthread_mutex_unlock(&r->c_head_lock);
    printf("Get: Unlocked next head at %i for buffer with key %i value %i\n", 
        c_ind, bd->k, bd->v);

    // Deep copy bd to buffer at prod index
    bd->k = r->buffer[c_ind].k;
    bd->v = r->buffer[c_ind].v;
    bd->ready = r->buffer[c_ind].ready;
    bd->req_type = r->buffer[c_ind].req_type;
    bd->res_off = r->buffer[c_ind].res_off;

    // Block until tail
    while (next(r->c_tail) != c_ind) {
        printf("Get: Stalling for tail turn at %i for buffer with key %i value %i\n", 
            c_ind, bd->k, bd->v);
    }

    printf("Get: Aquiring tail lock at %i for buffer with key %i value %i\n", 
        c_ind, bd->k, bd->v);
    pthread_mutex_lock(&r->c_tail_lock);

    r->c_tail = next(r->c_tail);

    pthread_mutex_unlock(&r->c_tail_lock);
    printf("Get: Incremented tail to %i for buffer with key %i value %i\n", 
        r->c_tail, bd->k, bd->v);
    
}
