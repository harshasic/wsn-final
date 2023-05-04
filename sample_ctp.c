#include "contiki.h"
#include "net/ctp.h"

/* CTP callbacks */
static void ctp_callback(const void *ptr, uint8_t hdr_len, uint8_t data_len,
    uint8_t options);


PROCESS(ctp_process, "CTP");
AUTOSTART_PROCESSES(&ctp_process);


PROCESS_THREAD(ctp_process, ev, data)
{
  PROCESS_BEGIN();

  /* Initialize CTP */
  ctp_init();

  /* Set CTP callback */
  ctp_set_callback(ctp_callback);

  while(1) {
    /* Wait for event */
    PROCESS_WAIT_EVENT();
  }

  PROCESS_END();
}


static void ctp_callback(const void *ptr, uint8_t hdr_len, uint8_t data_len,
    uint8_t options)
{
  /* CTP packet received */
  const struct data_hdr *hdr = ptr;
  printf("Received CTP packet from %d.%d with sequence number %u\n",
      rimeaddr_node_addr.u8[0], rimeaddr_node_addr.u8[1], hdr->seqno);

  /* Send ACK packet */
  struct data_hdr ack_hdr = {
    .type = ACK_TYPE,
    .seqno = hdr->seqno
  };
  NETSTACK_NETWORK.output(NULL, &ack_hdr, sizeof(ack_hdr), NULL);
}
