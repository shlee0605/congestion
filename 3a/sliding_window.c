#include "rlib.h"
#include "sliding_window.h"
#include <stdlib.h>

// Queue implementation references
// http://www.sanfoundry.com/c-program-queue-using-linked-list/

int packet_queue_count = 0;
packet_t* sliding_window[128];
int sws_head = 0, sws_tail = 5;

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

void fill_window() {
	for(int i = sws_head; i < sws_tail; i++) {
		if(sliding_window[i] == NULL) {
			packet_t* p_next_packet_to_send = dequeue_packet();
			sliding_window[i] = p_next_packet_to_send;
			return;
		}
	}
}

void show_window(packet_t *buffer, int len) {
	for(int i = 0; i < len; i++) {
		buffer[i] = sliding_window[i + sws_head];
	}
}

void run_sw_periodic_helper() {
	memset(sliding_window, NULL, 0);
    while (1) {
        if () { // check if there's an empty slot in the sender window
            
            // and send

        }
    }
}