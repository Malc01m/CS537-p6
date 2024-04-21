#include <stdio.h>

#include "ring_buffer.h"

void print_bd(struct buffer_descriptor *bd) {
    printf("k: %i\n", bd->k);
    printf("v: %i\n", bd->v);
    printf("ready: %i\n", bd->ready);
    printf("res_off: %i\n", bd->res_off);
    printf("req_type: %i\n", bd->req_type);
}

int main(int argc, char *argv[]) {

    // One submit/get
    struct ring r_1;
    struct buffer_descriptor get_bd;
    struct buffer_descriptor submit_bd;
    submit_bd.k = 12;
    submit_bd.v = 8;
    submit_bd.ready = 1;
    submit_bd.res_off = 2;
    submit_bd.req_type = GET;

    init_ring(&r_1);
    ring_submit(&r_1, &submit_bd);
    ring_get(&r_1, &get_bd);

    if ((get_bd.k != submit_bd.k) ||
        (get_bd.v != submit_bd.v) ||
        (get_bd.ready != submit_bd.ready) ||
        (get_bd.res_off != submit_bd.res_off) ||
        (get_bd.req_type != submit_bd.req_type)) {
        printf("One submit/get test fail\n");
        print_bd(&submit_bd);
        print_bd(&get_bd);
        return 0;
    }

    // Multiple ring passes, immediate submit/get - one thread
    struct ring r_2;
    init_ring(&r_2);

    for (int i = 0; i < (RING_SIZE * 5); i++) {

        submit_bd.k = i;
        submit_bd.v = i;
        submit_bd.ready = i;
        submit_bd.req_type = i;
        submit_bd.res_off = i;

        ring_submit(&r_2, &submit_bd);
        ring_get(&r_2, &get_bd);

        if ((get_bd.k != submit_bd.k) ||
            (get_bd.v != submit_bd.v) ||
            (get_bd.ready != submit_bd.ready) ||
            (get_bd.res_off != submit_bd.res_off) ||
            (get_bd.req_type != submit_bd.req_type)) {
            printf("One submit/get test fail\n");
            print_bd(&submit_bd);
            print_bd(&get_bd);
            return 0;
        }
    }

    // One ring pass, submit all but one then get all but one - one thread
    struct ring r_3;
    init_ring(&r_3);

    for (int i = 0; i < (RING_SIZE - 1); i++) {

        submit_bd.k = i;
        submit_bd.v = i;
        submit_bd.ready = i;
        submit_bd.req_type = i;
        submit_bd.res_off = i;

        ring_submit(&r_3, &submit_bd);
    }
  
    for (int i = 0; i < (RING_SIZE - 1); i++) {

        ring_get(&r_3, &get_bd);

        if ((get_bd.k != i) ||
            (get_bd.v != i) ||
            (get_bd.ready != i) ||
            (get_bd.res_off != i) ||
            (get_bd.req_type != i)) {
            printf("Submit all/get all test fail\n");
            print_bd(&submit_bd);
            print_bd(&get_bd);
            return 0;
        }
    }

}
