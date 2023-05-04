#include "contiki.h"
#include "net/rime.h"
#include "net/rime/collect.h"
#include "net/rime/ctp.h"
#include "dev/button-sensor.h"
#include "dev/leds.h"
#include "dev/serial-line.h"
#include <stdio.h>
#include <string.h>

#define MAX_PAYLOAD_LEN 20

static void run_eer(const void* data, uint16_t datalen, const linkaddr_t *nexthop);

static uint16_t compute_eer(const void* data, uint16_t datalen)
{
  /* Dummy EER computation */
  uint16_t eer = datalen * 2;
  return eer;
}

static void send_data(const void* data, uint16_t datalen, const linkaddr_t *nexthop, uint16_t eer)
{
  struct ctp_plus_header ctp_data;
  memcpy(&ctp_data.data, data, datalen);
  ctp_data.data_len = datalen;
  ctp_data.eer = eer;
  NETSTACK_NETWORK.output(&ctp_data, nexthop);
}

static void recv(const struct collect_data *data, const linkaddr_t *source, const linkaddr_t *prevhop)
{
  uint16_t eer = data->eer;
  printf("Received data: ");
  printf("%.*s", (int)data->payload_len, (char*)data->payload);
  printf(" from %d.%d, prevhop %d.%d, EER %u\n", 
         source->u8[0], source->u8[1], prevhop->u8[0], prevhop->u8[1], eer);

  /* If EER computation is required, run it before forwarding the data */
  if(data->seqno % 5 == 0) {
    uint16_t eer = compute_eer(data->payload, data->payload_len);
    if(eer == 0xffff) {
      /* Could not compute EER */
      ctp_proto_send_failed();
      return;
    }
    run_eer(data->payload, data->payload_len, ctp_next_hop());
  } else {
    /* Forward the data without computing EER */
    send_data(data->payload, data->payload_len, ctp_next_hop(), 0);
  }
}

static const struct collect_callbacks callbacks = { recv };

PROCESS(example_ee_process, "EER Example Process");
AUTOSTART_PROCESSES(&example_ee_process);

PROCESS_THREAD(example_ee_process, ev, data)
{
  PROCESS_BEGIN();

  printf("EER Example Process started\n");

  /* Initialize CTP */
  ctp_init();
  collect_open(&callbacks, 130, COLLECT_ROUTER, COLLECT_CONF_WITH_AUTOACK);
  ctp_set_callbacks(&callbacks);

  SENSORS_ACTIVATE(button_sensor);

  while(1) {
    PROCESS_WAIT_EVENT_UNTIL(ev == sensors_event && data == &button_sensor);
    printf("Button pressed, sending data...\n");
    static int count = 0;
    char payload[MAX_PAYLOAD_LEN];
    snprintf(payload, MAX_PAYLOAD_LEN, "Hello, World %d!", count);
    collect_send(payload, strlen(payload));
    count++;
  }

  PROCESS_END();
}

static void run_eer(const void* data, uint16_t datalen, const linkaddr_t *nexthop)
{
  uint16_t eer = compute_eer(data, datalen);
  send_data(data, datalen, nexthop, eer);
}
