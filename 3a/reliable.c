
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <stddef.h>
#include <assert.h>
#include <poll.h>
#include <errno.h>
#include <time.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <sys/uio.h>
#include <netinet/in.h>

#include "rlib.h"

#define PACKET_SIZE 500
#define HEADER_SIZE 12
#define PAYLOAD_SIZE 500
#define ACK_PACKET_SIZE 8

void set_network_bytes_and_checksum(packet_t* pkt); 
void send_ack_packet(rel_t* r);

struct reliable_state {
  rel_t *next;			/* Linked list for traversing all connections */
  rel_t **prev;

  conn_t *c;			/* This is the connection object */

  /* Add your own data fields below this */
  const struct config_common *cc;
  int read_eof; /* 1 - received eof, 0 - not received eof */
};
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
  r->read_eof = 0;
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
  //print_pkt (pkt, "recv", n);
  
  //add packet to receiver's slide window queue
  conn_output(r->c, (void*) pkt->data, n);
}


void
rel_read (rel_t *s)
{
  char buf[PAYLOAD_SIZE];
  //initialize a packet
  while(1) {
      
    // call conn_input to get the data to send in the packets
    int bytes_read = conn_input(s->c, buf, PAYLOAD_SIZE);
    
    // no data is available
    if(bytes_read == 0 || (bytes_read == -1 && s->read_eof == 1)) {
      // According to the instructions, conn_input() should return 0
      // when no data is available, but it seems it actually returns -1
      // because read() is causing EAGAIN for some reason.
      return;
    }

    packet_t *pkt = (packet_t*)malloc(sizeof(packet_t));
    int pkt_len = HEADER_SIZE;

    if(bytes_read == -1) {
      s->read_eof = 1;
      pkt_len = HEADER_SIZE;
    }
    else if(bytes_read > 0) {
      pkt_len = HEADER_SIZE + bytes_read;
      memset(pkt->data, 0, 500);        
      memcpy(pkt->data, buf, 500);
    }  

    pkt->cksum = 0;
    pkt->ackno = 0;
    pkt->seqno = 0;
    pkt->len = pkt_len;

    // send packet to sliding window queue
    set_network_bytes_and_checksum(pkt); 
    //print_pkt (pkt, "send", pkt_len);
    conn_sendpkt(s->c, pkt, pkt_len);

    // packet needs to be freed after getting acknowledgement
  }
}

void
rel_output (rel_t *r)
{
  
}

void
rel_timer ()
{
  /* Retransmit any packets that need to be retransmitted */

}

/* This function sets the packet in network byte order and calculates checksum.
   This should be called before sending a packet (conn_sendpkt) */
void set_network_bytes_and_checksum(packet_t* pkt) {
  int packet_length = (int)pkt->len;
  pkt->len = htons(pkt->len);
  pkt->ackno = htonl(pkt->ackno);
  if(packet_length != ACK_PACKET_SIZE) {
    pkt->seqno = htonl(pkt->seqno);
  }
  pkt->cksum = cksum((void*)pkt, packet_length);
}

void send_ack_packet(rel_t* r) {
  packet_t *pkt = (packet_t*) malloc(sizeof(packet_t));
  pkt->len = ACK_PACKET_SIZE;
  //TODO: set the correct ackno value.
  pkt->ackno = 0;
  pkt->cksum = 0;
  
  set_network_bytes_and_checksum(pkt);

  print_pkt((void *)pkt, "ack", ACK_PACKET_SIZE); 
  //enqueue packet to sliding window? or directly send it?
  conn_sendpkt(r->c, pkt, ACK_PACKET_SIZE);
}

