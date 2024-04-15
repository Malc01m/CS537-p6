#include <stdio.h>
#include "ring_buffer.h"

int main() {

    struct ring test_ring;
    init_ring(&test_ring);

    printf("Consumer tail: %i\n", test_ring.c_tail);
    printf("Consumer head: %i\n", test_ring.c_head);
    printf("Producer tail: %i\n", test_ring.p_tail);
    printf("Producer head: %i\n", test_ring.p_head);
    printf("Consumer tail prev: %i\n", prev(test_ring.c_tail));
    printf("Consumer tail next(prev): %i\n", next(prev(test_ring.c_tail)));
}