#ifndef _CONGESTION_SLIDING_WINDOW_H_
#define _CONGESTION_SLIDING_WINDOW_H_

#define UNACKED INT32_MAX
#define SEQUENCE_SPACE_SIZE 512

/*
	sender side:
 		sliding_window: packets that have been sent out, stored in HOST byte order
		w_size = sender window size (SWS)
		left = last acknowledgement recieved (LAR)
		next_seqno = seqno to be assigned to the next packet to be transmitted
		right = last frame sent (LFS)
		slot_timestamps_ms = each n-th slot means the timestamp of when sliding_window[n] was sent out last
		* LFS - LAR <= SWS

	receiver side:
 		sliding_window: packets that have been received, stored in HOST byte order
		w_size = receive window size (RWS)
		left = largest acceptable frame (LAF)
		next_seqno = not used
		right = last frame received (LFR)
		slot_timestamps_ms = not used
		* LAF - LFR <= RWS
*/
struct sliding_window_info {
  packet_t sliding_window[SEQUENCE_SPACE_SIZE];
  uint64_t slot_timestamps_ms[SEQUENCE_SPACE_SIZE];
  int is_slot_sent[SEQUENCE_SPACE_SIZE];
  int w_size;
  int left;
  int next_seqno;
  int right;
  int highest_acked_pkt;
};
typedef struct sliding_window_info sw_t;

uint64_t get_cur_time_ms();
void sw_recv_ack(const rel_t* p_rel, int seq_to_ack);
void sw_recv_packet(const rel_t* p_rel, const packet_t* p_packet);
void sw_send_window(const rel_t* p_rel);
void sw_store_packet(const rel_t* p_rel, const packet_t* p_packet);
int sw_should_sender_slot_resend(const rel_t* p_rel, int slot_idx);

#endif //_CONGESTION_SLIDING_WINDOW_H_
