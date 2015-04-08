#include <assert.h>
#include "reliable.h"
#include <string.h>
// stdio.h is used by DEBUG macros
#include <stdio.h>
#include <time.h>

void check_receiver_invariant(const sw_t* p_sw);
void check_sender_invariant(const sw_t* p_sw);
uint64_t get_cur_time_ms();
void send_ack_packet(const rel_t* r, uint32_t ackno);
void sw_recv_ack(const rel_t* p_rel, int seq_to_ack);
void sw_recv_packet(const rel_t* p_rel, const packet_t* p_packet);
void sw_send_packet(const rel_t* p_rel, const packet_t* p_packet);
int sw_should_sender_slot_resend(const rel_t* p_rel, int slot_idx);

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

uint64_t get_cur_time_ms() {
  return (clock() / CLOCKS_PER_SEC) * 1000;
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

  print_pkt(&processed_pkt, "sending ack", ACK_PACKET_SIZE);
  conn_sendpkt(r->c, &processed_pkt, ACK_PACKET_SIZE);
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
  // p_sw->right should not be updated here (it is updated in sw_send_packet)
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
    p_sw->highest_acked_pkt = highest_acked_packet;
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
  DEBUG("Checking invariant before the sliding windows sends a packet out...");
  DEBUG("left=%d right=%d w_size=%d", p_sw->left, p_sw->right, p_sw->w_size);
  check_sender_invariant(p_sw);

  if (p_sw->right - p_sw->left > p_sw->w_size) {
    DEBUG("Sending the packet will cause an invariant violation, not sending.");
    // Whenever a packet is sent out, p_sw->right will be incremented.
    // If this will cause an invariant violation,
    // do not send the packet.
    return;
  }

  packet_t* p_new_packet_in_sw = &(p_sw->sliding_window[p_sw->next_seqno]);
  p_new_packet_in_sw->seqno = (uint32_t) p_sw->next_seqno;
  p_new_packet_in_sw->ackno = p_packet->ackno;
  p_new_packet_in_sw->cksum = p_packet->cksum;
  p_new_packet_in_sw->len = p_packet->len;
  memcpy(p_new_packet_in_sw->data, p_packet->data, PAYLOAD_SIZE);
  packet_t network_ready_packet;
  set_network_bytes_and_checksum(&network_ready_packet, p_new_packet_in_sw);
  p_sw->next_seqno += 1;

  p_sw->slot_timestamps_ms[p_new_packet_in_sw->seqno] = get_cur_time_ms();

  print_pkt (&network_ready_packet, "sending", p_packet->len); // for debugging
  conn_sendpkt(p_rel->c, &network_ready_packet, p_packet->len);
  if (p_sw->right < SEQUENCE_SPACE_SIZE - 1) {
    p_sw->right += 1;
  } else {
    p_sw->right = SEQUENCE_SPACE_SIZE - 1;
  }
  p_new_packet_in_sw->ackno = UNACKED;
  // p_sw->left should not be updated here (it is updated in sw_recv_ack)
}

int sw_should_sender_slot_resend(const rel_t* p_rel, int slot_idx) {
  sw_t* p_sw = p_rel->sw_sender;
  int timeout = p_rel->cc->timeout;
  uint64_t cur_time_ms = get_cur_time_ms();
  if (cur_time_ms - p_sw->slot_timestamps_ms[slot_idx] > timeout) {
    packet_t* p_packet = &(p_sw->sliding_window[slot_idx]);
    if (p_packet->ackno == UNACKED) {
      return 1;
    }
  }
  return 0;
}
