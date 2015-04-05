#include "reliable.h"
#include "sliding_window.h"
#include <stdlib.h>
#include <string.h>

// Queue implementation references
// http://www.sanfoundry.com/c-program-queue-using-linked-list/

#define UNACKED INT32_MAX
#define WINDOW_SIZE 5

int packet_queue_count = 0;
rel_t* p_rel = NULL;
packet_t* sliding_window[128];
int sws_head = 0, sws_tail = WINDOW_SIZE;

struct packet_node {
    packet_t *info;
    struct packet_node *ptr;
} *front, *rear, *temp, *front1;

struct sliding_window_info {
    packet_t* sliding_window;
    int w_size;
    uint32_t seq_num;
    uint32_t ack_num;
}
typedef struct sliding_window_info sw_t;

packet_t *dequeue_packet() {
    front1 = front;
    if (front1 == NULL) {
        return NULL;
    }
    else if (front1->ptr != NULL) {
        front1 = front1->ptr;
        packet_t *rtn = front->info;
        rtn->ackno = UNACKED;
        free(front);
        front = front1;
        packet_queue_count--;
        return rtn;
    }
    else {
        packet_t *rtn = front->info;
        rtn->ackno = UNACKED;
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
		}
	}
}

void show_window(packet_t *buffer[], int len) {
	for(int i = 0; i < len; i++) {
		buffer[i] = sliding_window[i + sws_head];
	}
}

void sw_set_reliable(rel_t* p) {
    p_rel = p;
}

/// Returns 1 if every packet in the current window has been ACKed.
/// Returns 0 otherwise.
int is_window_all_acked() {
    for (int i = sws_head; i < sws_tail; ++i) {
        if (sliding_window[i]->ackno == UNACKED) {
            return 1;
        }
    }
    return 0;
}

void slide_window(int byHowMany) {
    sws_head += byHowMany;
    sws_tail += byHowMany;
}

void send_ack_packet(rel_t* r, uint32_t ackno) {
    packet_t *pkt = (packet_t*) malloc(sizeof(packet_t));
    pkt->len = ACK_PACKET_SIZE;
    //TODO: set the correct ackno value.
    pkt->ackno = ackno;
    pkt->cksum = 0;

    set_network_bytes_and_checksum(pkt);

    print_pkt((void *)pkt, "ack", ACK_PACKET_SIZE);
    conn_sendpkt(r->c, pkt, ACK_PACKET_SIZE);
}

void sw_run_periodic_helper() {
	memset(sliding_window, NULL, 0);

    fill_window();
    conn_sendpkt(p_rel->c, sliding_window, WINDOW_SIZE);
    while (1) {
        if (is_window_all_acked()) {
            slide_window(WINDOW_SIZE);
            fill_window();
            conn_sendpkt(p_rel->c, sliding_window, WINDOW_SIZE);
        }

        // TODO: receive ACKs here
    }
}
