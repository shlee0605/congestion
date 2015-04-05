#include <assert.h>
#include "reliable.h"
#include <string.h>
#include <stdlib.h>

void check_receiver_invariant(const sw_t* p_sw);
void send_ack_packet(const rel_t* r, uint32_t ackno);
void sw_recv_packet(const rel_t* p_rel, const packet_t* p_packet);

void check_receiver_invariant(const sw_t* p_sw) {
  int laf_idx = p_sw->right;
  int lfr_idx = p_sw->left;
  int window_size = p_sw->w_size;
  assert(laf_idx - lfr_idx <= window_size);
}

void send_ack_packet(const rel_t* r, uint32_t ackno) {
  packet_t* pkt = (packet_t*) malloc(sizeof(packet_t));
  pkt->len = ACK_PACKET_SIZE;
  pkt->ackno = ackno;
  pkt->cksum = 0;

  set_network_bytes_and_checksum(pkt);

  print_pkt((void*) pkt, "ack", ACK_PACKET_SIZE);
  conn_sendpkt(r->c, pkt, ACK_PACKET_SIZE);
}

void sw_recv_packet(const rel_t* p_rel, const packet_t* p_packet) {
  sw_t* p_sw = p_rel->sw_receiver;
  check_receiver_invariant(p_sw);

  // word-by-word translation of Computer Networks: a Systems Approach p.108-109

  // When a frame with sequence number SeqNum arrives,
  int seq_num = p_packet->seqno;
  // If SeqNum ≤ LFR or SeqNum > LAF,
  int laf = p_sw->right;
  int lfr = p_sw->left;
  if (seq_num <= lfr && seq_num > laf) {
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
  strcpy(p_slot->data, p_packet->data);
  if (p_packet->seqno == seq_num_to_ack) {
    // If the received packet *is* SeqNumToAck,
    // send an ACK for all received packets contiguous to SeqNumToAck.
    int i = seq_num_to_ack;
    while (i < SEQUENCE_SPACE_SIZE &&
           p_sw->sliding_window[i].ackno != UNACKED) {
      send_ack_packet(p_rel, (uint32_t) i);
      i += 1;
    }
    // It then updates LFR and LAF.
    lfr = p_sw->left = i - 1;
    laf = p_sw->right = lfr + p_sw->w_size;
  }
}
