
#include <stdint.h>

typedef struct header {
    uint32_t conn_id;
    uint32_t seq_num;
    uint32_t ack_num;
    uint32_t checksum;
    uint16_t payload;
    uint8_t flags;
    uint8_t  padding = 0;


} ProtocolHeader, *ProtocolHeaderPtr;