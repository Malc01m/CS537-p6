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
    
    uint32_t loc_p_head = r->p_head;
    uint32_t loc_c_tail = r->c_tail;
    uint32_t prod_next = next(loc_p_head);

    // Still haven't figured out how to use this, might switch to mutex or semaphore
    atomic_compare_exchange_strong(&(r->p_head), &loc_p_head, &loc_p_head);
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
    atomic_compare_exchange_strong();
}