
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

struct reliable_state {
  rel_t *next;			/* Linked list for traversing all connections */
  rel_t **prev;

  conn_t *c;			/* This is the connection object */

  /* Add your own data fields below this */
  const struct config_common *cc;
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
  print_pkt (pkt, "recv", n);
}


void
rel_read (rel_t *s)
{
  //initialize a packet
  packet_t *pkt;
  pkt = xmalloc(sizeof(pkt));
  memset(pkt->data, 0, PAYLOAD_SIZE);
  
  // call conn_input to get the data to send in the packets
  int bytes_read = conn_input(s->c, pkt->data, PAYLOAD_SIZE);
  
  // no data is available
  if(bytes_read == 0) {
    free(pkt);
    // According to the instructions, conn_input() should return 0
    // when no data is available, but it seems it actually returns -1
    // because read() is causing EAGAIN for some reason.
    return;
  }
  
  if(bytes_read == -1) {
    pkt->len = HEADER_SIZE;
  }
  else {
    pkt->len = HEADER_SIZE + bytes_read;
  }
      
  pkt->cksum = 0;
  pkt->ackno = 0;
  pkt->seqno = 0;

  // send packet to sliding window queue
  print_pkt (pkt, "send", pkt->len);
  conn_sendpkt(s->c, pkt, pkt->len);
  //free(pkt);
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
