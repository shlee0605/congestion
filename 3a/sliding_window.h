#ifndef _CONGESTION_SLIDING_WINDOW_H_
#define _CONGESTION_SLIDING_WINDOW_H_

#define UNACKED INT32_MAX
#define SEQUENCE_SPACE_SIZE 128

/*
	sender side:
		w_size = sender window size (SWS)
		left = last frame sent (LFS)
		next_seqno = seqno to be assigned to the next packet to be transmitted
		right = last acknowledgement recieved (LAR)
		* LFS - LAR <= SWS

	receiver side:
		w_size = receive window size (RWS)
		left = largest acceptable frame (LAF)
		next_seqno = not used
		right = last frame received (LFR)
		* LAF - LFR <= RWS
*/
struct sliding_window_info {
  packet_t sliding_window[128];
  int w_size;
  int left;
  int next_seqno;
  int right;
  int highest_acked_pkt;
};
typedef struct sliding_window_info sw_t;

void sw_recv_ack(const rel_t* p_rel, int seq_to_ack);
void sw_recv_packet(const rel_t* p_rel, const packet_t* p_packet);
void sw_send_packet(const rel_t* p_rel, const packet_t* p_packet);

#endif //_CONGESTION_SLIDING_WINDOW_H_
