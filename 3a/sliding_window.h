#ifndef _CONGESTION_SLIDING_WINDOW_H_
#define _CONGESTION_SLIDING_WINDOW_H_

#define UNACKED INT32_MAX
#define SEQUENCE_SPACE_SIZE 128

/*
	sender side:
		w_size = sender window size (SWS)
		left = last frame sent (LFS)
		right = last acknowledgement recieved (LAR)
		* LFS - LAR <= SWS

	receiver side:
		w_size = receive window size (RWS)
		left = largest acceptable frame (LAF)
		right = last frame received (LFR)
		* LAF - LFR <= RWS
*/
struct sliding_window_info {
  packet_t sliding_window[128];
  int w_size;
  uint32_t left;
  uint32_t right;
};
typedef struct sliding_window_info sw_t;

void sw_recv_packet(const rel_t* p_rel, const packet_t* p_packet);

#endif //_CONGESTION_SLIDING_WINDOW_H_
