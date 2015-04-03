#include <assert.h>
#include "rlib.h"
#include "sliding_window.h"

void queue_tests() {
    assert(dequeue_packet() == NULL); // when queue is empty, dequeue_packet() should return NULL

    packet_t p1;
    p1.seqno = 1;
    packet_t p2;
    p2.seqno = 2;
    packet_t p3;
    p3.seqno = 3;

    enqueue_packet(&p1);
    assert(dequeue_packet() == &p1); // make sure pointer is left intact
    assert(p1.seqno == 1); // make sure content of packet doesn't change
    assert(dequeue_packet() == NULL); // make sure queue is empty againt

    enqueue_packet(&p1);
    enqueue_packet(&p2);
    enqueue_packet(&p3);
    assert(dequeue_packet() == &p1); // make sure FIFO
    assert(dequeue_packet() == &p2); // make sure FIFO
    assert(dequeue_packet() == &p3); // make sure FIFO
    assert(dequeue_packet() == NULL); // make sure queue is empty again
}

int main() {
    queue_tests();

    return 0;
}