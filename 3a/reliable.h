#include "rlib.h"
#include "sliding_window.h"

#define HEADER_SIZE 12
#define PAYLOAD_SIZE 500
#define ACK_PACKET_SIZE 8

struct reliable_state {
    rel_t *next;			/* Linked list for traversing all connections */
    rel_t **prev;

    conn_t *c;			/* This is the connection object */

    /* Add your own data fields below this */
    const struct config_common *cc;
    int file_eof; /* 1 - received eof, 0 - not received eof */
    sw_t* sw_sender;
    sw_t* sw_receiver;
};

void rel_destroy (rel_t *r);

void rel_recvpkt (rel_t *r, packet_t *pkt, size_t n);

void rel_read (rel_t *s);

void rel_output (rel_t *r);

void rel_timer ();

void set_network_bytes_and_checksum(packet_t* pkt);
