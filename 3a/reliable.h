#include "rlib.h"
#include "sliding_window.h"

#ifdef NDEBUG
    #define DEBUG(M, ...)
#else
    #define DEBUG(M, ...) fprintf(stderr, "[DEBUG] %s:%d: " M "\n", __FILE__, __LINE__, ##__VA_ARGS__)
#endif

#define HEADER_SIZE 12
#define EOF_PACKET_SIZE HEADER_SIZE
#define PAYLOAD_SIZE 500
#define ACK_PACKET_SIZE 8
#define EOF_PACKET_SIZE 12
#define EOF_ACK_TAG 999

struct reliable_state {
    rel_t *next;			/* Linked list for traversing all connections */
    rel_t **prev;

    conn_t *c;			/* This is the connection object */

    /* Add your own data fields below this */
    const struct config_common *cc;
    int file_eof; /* 1 - received eof, 0 - not received eof */
    sw_t* sw_sender;
    sw_t* sw_receiver;
    int written[SEQUENCE_SPACE_SIZE];
    int eof_ack_received;
    int eof_received;
    int last_pkt_num;
};

void rel_destroy (rel_t *r);

void rel_recvpkt (rel_t *r, packet_t *pkt, size_t n);

void rel_read (rel_t *s);

void rel_output (rel_t *r);

void rel_timer ();

void set_network_bytes_and_checksum(packet_t* dst, const packet_t* src);
