#include "reliable.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/socket.h>

void set_network_bytes_and_checksum(packet_t* dst, const packet_t* src);
void set_host_bytes(packet_t* pkt);
void initialize_receiver_sw_info(const struct config_common *cc, sw_t* sliding);
void initialize_sender_sw_info(const struct config_common *cc, sw_t* sliding);
int all_pkts_written(rel_t* r);
struct reliable_state;
rel_t *rel_list;

/* Creates a new reliable protocol session, returns NULL on failure.
 * Exactly one of c and ss should be NULL.  (ss is NULL when called
 * from rlib.c, while c is NULL when this function is called from
 * rel_demux.) */
rel_t *
rel_create (conn_t *c, const struct sockaddr_storage *ss,
	    const struct config_common *cc)
{
  rel_t *r;

  r = xmalloc (sizeof (*r));
  memset (r, 0, sizeof (*r));

  if (!c) {
    c = conn_create (r, ss);
    if (!c) {
      free (r);
      return NULL;
    }
  }

  r->c = c;
  r->next = rel_list;
  r->prev = &rel_list;
  if (rel_list)
    rel_list->prev = &r->next;
  rel_list = r;

  /* Do any other initialization you need here */
  r->cc = cc;
  r->file_eof = 0;
  r->sw_sender = (sw_t*)malloc(sizeof(sw_t));
  r->sw_receiver = (sw_t*)malloc(sizeof(sw_t));
  initialize_sender_sw_info(r->cc, r->sw_sender);
  initialize_receiver_sw_info(r->cc, r->sw_receiver);
  int i;
  for(i = 0; i < SEQUENCE_SPACE_SIZE; i ++) {
    r->written[i] = 0;
  }
  r->eof_ack_received = 0;
  r->eof_received = 0;
  r->last_pkt_num = 0;
  return r;
}

void
rel_destroy (rel_t *r)
{
  if (r->next)
    r->next->prev = r->prev;
  *r->prev = r->next;
  conn_destroy (r->c);

  /* Free any other allocated memory here */
}


/* This function only gets called when the process is running as a
 * server and must handle connections from multiple clients.  You have
 * to look up the rel_t structure based on the address in the
 * sockaddr_storage passed in.  If this is a new connection (sequence
 * number 1), you will need to allocate a new conn_t using rel_create
 * ().  (Pass rel_create NULL for the conn_t, so it will know to
 * allocate a new connection.)
 */
void
rel_demux (const struct config_common *cc,
	   const struct sockaddr_storage *ss,
	   packet_t *pkt, size_t len)
{
}

void
rel_recvpkt (rel_t *r, packet_t *pkt, size_t n)
{
  print_pkt (pkt, "new packet received", (int) n); // for debugging purposes
  // check if the packet is valid using checksum.
  uint16_t original = pkt->cksum;
  pkt->cksum = 0;
  uint16_t new = cksum((void*)pkt, (int) n);
  set_host_bytes(pkt);

  if(original == new) {
    if (pkt->len == ACK_PACKET_SIZE) {
      DEBUG("it is an ACK packet with ackno=%d seqno=%d", pkt->ackno, pkt->seqno);
      if(pkt->ackno > SEQUENCE_SPACE_SIZE) {
        uint32_t decoded_ackno = pkt->ackno / SEQUENCE_SPACE_SIZE;
        r->eof_ack_received = 1;
        r->last_pkt_num = decoded_ackno - 2;
        DEBUG("EOF's ACK received, ackno=%d", decoded_ackno);
        sw_recv_ack(r, decoded_ackno);
        rel_output(r);
      } else {
        sw_recv_ack(r, pkt->ackno);
      }
    } else {
      DEBUG("it is a data packet");
      if(pkt->len == EOF_PACKET_SIZE) {
        DEBUG("EOF packet received!");
        r->eof_received = 1;
      }
      sw_recv_packet(r, pkt);
      rel_output(r);
    }
  }
  else {
    DEBUG("pkt dropped");
  }
}

void
rel_read (rel_t *s)
{
  char buf[PAYLOAD_SIZE];
  //initialize a packet
  while(1) {
    memset(buf, 0, PAYLOAD_SIZE);
    // call conn_input to get the data to send in the packets
    int bytes_read = conn_input(s->c, buf, PAYLOAD_SIZE);

    // no data is available
    if(bytes_read == 0 || (bytes_read == -1 && s->file_eof == 1)) {
      // When a keyboard input is pushed into stdin,
      // EOF is never hit and bytes_read never becomes -1
      // (instead it becomes 0 on the second round).
      // But when a file input is redirected into stdin,
      // EOF is hit and bytes_read does become -1.
      // Furthermore, without the new condition,
      // this while loop runs multiple times with bytes_read being -1.
      // To make sure the loop stops immediately after
      // bytes_read becomes -1 for the first time,
      // the new conditions is placed.

      // According to the instructions, conn_input() should return 0
      // when no data is available, but it seems it actually returns -1
      // because read() is causing EAGAIN for some reason.
      return;
    }

    packet_t pkt;
    int pkt_len = HEADER_SIZE; // This initialization means:
    // when an EOF is read in, send an EOF packet
    // (a packet that marks the end of a message).
    // Notice how it is unchanged in the following if block.

    if(bytes_read == -1) {
      s->file_eof = 1;
      // see the comment inside the preceding if block with the condition
      // bytes_read == 0 || (bytes_read == -1 && s->file_eof == 1
    }
    if(bytes_read > 0) {
      pkt_len += bytes_read;
      memset(pkt.data, 0, PAYLOAD_SIZE);
      memcpy(pkt.data, buf, PAYLOAD_SIZE);
    }

    pkt.cksum = 0;
    pkt.ackno = 0;
    pkt.seqno = 0;
    pkt.len = (uint16_t) pkt_len;

    sw_store_packet(s, &pkt);
    sw_send_window(s);
  }
}

void
rel_output (rel_t *r)
{
  int high = r->sw_receiver->highest_acked_pkt;
  int i;
  for(i = 1; i <= high; i++) {
    if(r->written[i] == 0) {
      packet_t* pkt = &(r->sw_receiver->sliding_window[i]);
      // if(r->eof_ack_received == 1 && r->eof_received == 1 && r->file_eof == 1) {
      //   DEBUG("eof ack received  / eof received");
      //   if(all_pkts_written(r) == 1) {
      //     conn_output(r->c, NULL, 0);
      //     r->written[i];
      //   }
      // }
      if(r->eof_ack_received == 1 && r->eof_received == 1) {
        int pid = getpid ();
        DEBUG("EOF_ACK_PKT, EOF_PKT received: %d", pid);
        conn_output(r->c, NULL, 0);
      }
      else if(r->eof_ack_received == 1 || r->eof_received ==1) {
        DEBUG("Only one of EOF and EOF ACK is received.");
      }
      else {
        conn_output(r->c, pkt->data, pkt->len - HEADER_SIZE);
        r->written[i] = 1;
        //the following lines are for debug purpose
        packet_t pkt_to_print;
        set_network_bytes_and_checksum(&pkt_to_print, pkt);
        print_pkt(&pkt_to_print, "print to stdout", pkt->len);
      }
    }
  }
}

void
rel_timer ()
{
  /* Retransmit any packets that need to be retransmitted */
  sw_t* p_sw = rel_list->sw_sender;
  int slot_idx;
  for (slot_idx = 1; slot_idx <= p_sw->right; ++slot_idx) {
    if (sw_should_sender_slot_resend(rel_list, slot_idx)) {
      DEBUG("rel_timer: slot_idx=%d left=%d right=%d", slot_idx, p_sw->left, p_sw->right);
      packet_t* p_packet = &(p_sw->sliding_window[slot_idx]);
      packet_t pkt_to_send;
      set_network_bytes_and_checksum(&pkt_to_send, p_packet);
      print_pkt(&pkt_to_send, "retransmission", p_packet->len);
      conn_sendpkt(rel_list->c, &pkt_to_send, p_packet->len); // resend
      p_sw->slot_timestamps_ms[slot_idx] = get_cur_time_ms(); // update timer
    }
  }
}

/// This function generates a new packet in network byte order
/// and with the checksum calculated.
/// It should be called before sending a packet (conn_sendpkt)
void set_network_bytes_and_checksum(packet_t* dst, const packet_t* src) {
  int packet_length = (int) src->len;
  dst->len = htons(src->len);
  dst->ackno = htonl(src->ackno);
  dst->seqno = htonl(src->seqno);
  DEBUG("set_network_bytes_and_checksum: ackno=%d, seqno=%d", ntohl(dst->ackno), ntohl(dst->seqno));
  memset(dst->data, 0, PAYLOAD_SIZE);
  memcpy(dst->data, src->data, PAYLOAD_SIZE);
  dst->cksum = 0;
  dst->cksum = cksum((void*) dst, src->len);
}

void set_host_bytes(packet_t* pkt) {
  pkt->len = ntohs(pkt->len);
  pkt->ackno = ntohl(pkt->ackno);
  pkt->seqno = ntohl(pkt->seqno);
}

void initialize_receiver_sw_info(const struct config_common *cc, sw_t* sliding) {
  sliding->w_size = cc->window;
  sliding->left = 0;
  sliding->next_seqno = 1;
  sliding->right = sliding->left + sliding->w_size;
  int i;
  for (i = 0; i < SEQUENCE_SPACE_SIZE; ++i) {
    sliding->sliding_window[i].ackno = UNACKED;
  }
}

void initialize_sender_sw_info(const struct config_common *cc, sw_t* sliding) {
  sliding->w_size = cc->window;
  sliding->left = 0;
  sliding->next_seqno = 1;
  sliding->right = 0;
  int i;
  for (i = 0; i < SEQUENCE_SPACE_SIZE; ++i) {
    sliding->sliding_window[i].ackno = UNACKED;
  }
  memset(sliding->is_slot_sent, 0, sizeof(int) * SEQUENCE_SPACE_SIZE);
}

int all_pkts_written(rel_t* r) {
  int i;
  for(i = 0; i < r->last_pkt_num; i++) {
    if(r->written[i] == 0) {
      return 0;
    }
  }
  return 1;
}