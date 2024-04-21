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
    // Consumer will start off just behind the producer
	r->c_tail = 0;
	r->c_head = 0;
    r->p_tail = 1;
	r->p_head = 1;
    for (int i = 0; i < RING_SIZE; i++) {
        r->buffer[i].k = 0;
        r->buffer[i].v = 0;
        r->buffer[i].ready = 0;
        r->buffer[i].req_type = 0;
        r->buffer[i].res_off = 0;
    }

    if (pthread_mutex_init(&r->c_head_lock, NULL) != 0) { 
        printf("Could not initialize mutex lock\n"); 
        return -1;
    } 
    if (pthread_mutex_init(&r->p_head_lock, NULL) != 0) { 
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
    
    printf("Put: Aquiring lock...\n");
    pthread_mutex_lock(&r->p_head_lock);
    
    r->p_head = next(r->p_head);
    uint32_t p_ind = r->p_head;

    pthread_mutex_unlock(&r->p_head_lock);
    printf("Put: Unlocked with next head at %i\n", p_ind);

    // Block on full
    while (r->c_tail == p_ind) {};

    // Deep copy bd to buffer at prod index
    r->buffer[p_ind].k = bd->k;
    r->buffer[p_ind].v = bd->v;
    r->buffer[p_ind].ready = bd->ready;
    r->buffer[p_ind].req_type = bd->req_type;
    r->buffer[p_ind].res_off = bd->res_off;

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

    printf("Get: Aquiring lock...\n");
    pthread_mutex_lock(&r->c_head_lock);

    r->c_head = next(r->c_head);
    uint32_t c_ind = r->c_head;

    pthread_mutex_unlock(&r->c_head_lock);
    printf("Get: Unlocked with next head at %i\n", c_ind);

    // Block on empty
    while (r->p_tail == c_ind) {};

    // Deep copy bd to buffer at prod index
    bd->k = r->buffer[next(r->c_head)].k;
    bd->v = r->buffer[next(r->c_head)].v;
    bd->ready = r->buffer[next(r->c_head)].ready;
    bd->req_type = r->buffer[next(r->c_head)].req_type;
    bd->res_off = r->buffer[next(r->c_head)].res_off;
    
}
