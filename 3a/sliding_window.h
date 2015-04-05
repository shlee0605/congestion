#ifndef _CONGESTION_SLIDING_WINDOW_H_
#define _CONGESTION_SLIDING_WINDOW_H_

struct sliding_window_info {
    packet_t sliding_window[128];
    int w_size;
    uint32_t seq_num;
    uint32_t ack_num;
};
typedef struct sliding_window_info sw_t;


/// Returns the packet_t* in the front of the queue.
/// Returns NULL if the queue is empty.
packet_t *dequeue_packet();

/// Enqueues a new packet to the end of the queue.
/// The packet will be picked up by the Sliding Window algorithm
/// as soon as possible.
void enqueue_packet(packet_t *p_packet);

void fill_window();

void show_window(packet_t* buffer[], int len);

#endif //_CONGESTION_SLIDING_WINDOW_H_