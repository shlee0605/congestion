#include <assert.h>
#include "rlib.h"
#include "sliding_window.h"

#define WINDOW_SIZE 5

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

    assert(dequeue_packet() == NULL); // make sure queue is empty by the end of this function (queue is global)
}

void fill_window_tests() {
    assert(dequeue_packet() == NULL); // make sure queue is empty initially

    packet_t packets[WINDOW_SIZE];
    for (uint32_t i = 0; i < WINDOW_SIZE; ++i) {
        packet_t p;
        p.seqno = i;
        packets[i] = p;
    }

    packet_t* window_buf[WINDOW_SIZE];

    fill_window();
    show_window(window_buf, WINDOW_SIZE);
    for (int i = 0; i < WINDOW_SIZE; ++i) {
        assert(window_buf[i] == NULL);
    }

    enqueue_packet(&packets[0]);
    fill_window();
    show_window(window_buf, WINDOW_SIZE);
    assert(window_buf[0] == &packets[0]);
    assert(dequeue_packet() == NULL);
    for (int i = 1; i < WINDOW_SIZE; ++i) {
        assert(window_buf[i] == NULL);
    }

    for (int i = 1; i < WINDOW_SIZE; ++i) {
        enqueue_packet(&packets[i]);
    }
    fill_window();
    show_window(window_buf, WINDOW_SIZE);
    for (int i = 1; i < WINDOW_SIZE; ++i) {
        assert(window_buf[i] == &packets[i]);
    }
    fill_window();
    show_window(window_buf, WINDOW_SIZE);
    for (int i = 1; i < WINDOW_SIZE; ++i) {
        assert(window_buf[i] == &packets[i]);
    }
    assert(dequeue_packet() == NULL);
}

int main() {
    queue_tests();
    fill_window_tests();

    return 0;
}