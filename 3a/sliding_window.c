#include "rlib.h"
#include "sliding_window.h"
#include <stdlib.h>

// Queue implementation references
// http://www.sanfoundry.com/c-program-queue-using-linked-list/

int packet_queue_count = 0;

struct packet_node {
    packet_t *info;
    struct packet_node *ptr;
} *front, *rear, *temp, *front1;

packet_t *dequeue_packet() {
    front1 = front;
    if (front1 == NULL) {
        return NULL;
    }
    else if (front1->ptr != NULL) {
        front1 = front1->ptr;
        packet_t *rtn = front->info;
        free(front);
        front = front1;
        packet_queue_count--;
        return rtn;
    }
    else {
        packet_t *rtn = front->info;
        free(front);
        front = NULL;
        rear = NULL;
        packet_queue_count--;
        return rtn;
    }
}

/// Enqueues a new packet to the end of the queue.
/// The packet will be picked up by the Sliding Window algorithm
/// as soon as possible.
void enqueue_packet(packet_t *p_packet) {
    if (rear == NULL) {
        rear = (struct packet_node *) malloc(1 * sizeof(struct packet_node));
        rear->ptr = NULL;
        rear->info = p_packet;
        front = rear;
    }
    else {
        temp = (struct packet_node *) malloc(1 * sizeof(struct packet_node));
        rear->ptr = temp;
        temp->info = p_packet;
        temp->ptr = NULL;

        rear = temp;
    }
    packet_queue_count++;
}