
#include <stdint.h>
#include <string.h> // memset
#define HEADER_SIZE 24
#define MAX_PROTOCOL_SIZE HEADER_SIZE + 1024

typedef enum {
    SYN = 1,
    ACK = 2,
    FIN = 4,
    RST = 8,
    DATA = 16,
} FlagsEnum;

typedef struct header {
    uint32_t conn_id;
    uint32_t seq_num;
    uint32_t ack_num;
    uint16_t checksum;
    uint16_t payload_size;
    uint8_t  flags;
    uint16_t padding;
    uint8_t  padding2;
    char data[];
} ProtocolHeader, *ProtocolHeaderPtr;