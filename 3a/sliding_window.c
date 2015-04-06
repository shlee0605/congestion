#include <assert.h>
#include "reliable.h"
#include <string.h>

void check_receiver_invariant(const sw_t* p_sw);
void check_sender_invariant(const sw_t* p_sw);
void send_ack_packet(const rel_t* r, uint32_t ackno);
void sw_recv_ack(const rel_t* p_rel, int seq_to_ack);
void sw_recv_packet(const rel_t* p_rel, const packet_t* p_packet);
void sw_send_packet(const rel_t* p_rel, const packet_t* p_packet);

void check_receiver_invariant(const sw_t* p_sw) {
  int laf_idx = p_sw->right;
  int lfr_idx = p_sw->left;
  int window_size = p_sw->w_size;
  assert(laf_idx - lfr_idx <= window_size);
}

void check_sender_invariant(const sw_t* p_sw) {
  int lfs_idx = p_sw->right;
  int lar_idx = p_sw->left;
  int window_size = p_sw->w_size;
  assert(lfs_idx - lar_idx <= window_size);
}

void send_ack_packet(const rel_t* r, uint32_t ackno) {
  // If we send ACK packet with ackno=5,
  // that means we've successfully received 1~4 and we need 5 (TCP-style ACK).

  packet_t pkt;
  pkt.len = ACK_PACKET_SIZE;
  pkt.ackno = ackno;
  pkt.cksum = 0;
  pkt.seqno = 0;

  packet_t processed_pkt;
  set_network_bytes_and_checksum(&processed_pkt, &pkt);

  print_pkt(&processed_pkt, "sending ack", sizeof(processed_pkt));
  conn_sendpkt(r->c, &processed_pkt, sizeof(processed_pkt));
}

void sw_recv_ack(const rel_t* p_rel, int ackno) {
  // If we receive ACK packet with ackno=5,
  // that means we've successfully received 1~4 and we need 5 (TCP-style ACK).

  sw_t* p_sw = p_rel->sw_sender;
  int i;
  for (i = p_sw->left + 1; i < ackno; ++i) {
    p_sw->sliding_window[i].ackno = (uint32_t) ackno; // TODO: should it be set to this?
  }
  p_sw->left = ackno - 1;
  p_sw->right = p_sw->left + p_sw->w_size;
}

void sw_recv_packet(const rel_t* p_rel, const packet_t* p_packet) {
  sw_t* p_sw = p_rel->sw_receiver;
  check_receiver_invariant(p_sw);

  // word-by-word translation of Computer Networks: a Systems Approach p.108-109

  // When a frame with sequence number SeqNum arrives,
  int seq_num = p_packet->seqno;
  // If SeqNum ≤ LFR or SeqNum > LAF,
  int lfr = p_sw->left;
  int laf = p_sw->right;
  if (seq_num <= lfr || seq_num > laf) {
    // then the frame is outside the receiver’s window and it is discarded.
    return;
  }
  // If LFR < SeqNum ≤ LAF,
  // then the frame is within the receiver’s window and it is accepted.
  // Now the receiver needs to decide whether or not to send an ACK.
  // Let SeqNumToAck denote the largest sequence number not yet acknowledged,
  // such that all frames with sequence numbers
  // less than or equal to SeqNumToAck have been received.
  int seq_num_to_ack = lfr + 1;
  while (seq_num_to_ack < SEQUENCE_SPACE_SIZE &&
      p_sw->sliding_window[seq_num_to_ack].ackno != UNACKED) {
    seq_num_to_ack += 1;
  }
  // TODO: what if seq_num_to_ack == SEQUENCE_SPACE_SIZE ?
  // If the received packet is not SeqNumToAck,
  // accpet the packet but don't send an ACK.
  packet_t* p_slot = &(p_sw->sliding_window[p_packet->seqno]);
  p_slot->seqno = p_packet->seqno;
  p_slot->ackno = p_packet->ackno;
  p_slot->cksum = p_packet->cksum;
  p_slot->len = p_packet->len;
  memcpy(p_slot->data, p_packet->data, PAYLOAD_SIZE);
  if (p_packet->seqno == seq_num_to_ack) {
    // If the received packet *is* SeqNumToAck,
    // send an ACK for the highest received packet contiguous to SeqNumToAck.
    int highest_acked_packet = seq_num_to_ack;

    while (highest_acked_packet < SEQUENCE_SPACE_SIZE &&
        p_sw->sliding_window[highest_acked_packet].ackno != UNACKED) {
      highest_acked_packet += 1;
    }
    highest_acked_packet -= 1;
    int next_ackno = highest_acked_packet + 1;
    send_ack_packet(p_rel, (uint32_t) next_ackno);
    // It then updates LFR and LAF.
    p_sw->left = highest_acked_packet; // lfr
    if(p_sw->left + p_sw->w_size < SEQUENCE_SPACE_SIZE) {
      p_sw->right = p_sw->left + p_sw->w_size;  // laf
    } else {
      p_sw->right = SEQUENCE_SPACE_SIZE - 1;  //laf
    }
  }
}

void sw_send_packet(const rel_t* p_rel, const packet_t* p_packet) {
  sw_t* p_sw = p_rel->sw_sender;
  check_sender_invariant(p_sw);

  packet_t packet_with_new_seqno;
  packet_with_new_seqno.seqno = (uint32_t) p_sw->next_seqno;
  packet_with_new_seqno.ackno = p_packet->ackno;
  packet_with_new_seqno.cksum = p_packet->cksum;
  packet_with_new_seqno.len = p_packet->len;
  memcpy(packet_with_new_seqno.data, p_packet->data, PAYLOAD_SIZE);
  packet_t network_ready_packet;
  set_network_bytes_and_checksum(&network_ready_packet, &packet_with_new_seqno);
  p_sw->next_seqno += 1;

  print_pkt (&network_ready_packet, "sending", p_packet->len); // for debugging
  conn_sendpkt(p_rel->c, &network_ready_packet, p_packet->len);
  // Also, the sender associates a timer with each frame it transmits,
  // and it retransmits the frame should the timer expire before an ACK is received.
  // TODO
}
