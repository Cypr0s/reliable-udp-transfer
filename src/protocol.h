
#include <stdint.h> // types
#include <string.h> // memset
#include "error.h"  // exitcode

#define HEADER_SIZE 24
#define MAX_PROTOCOL_SIZE HEADER_SIZE + MAX_DATA_SIZE
#define MAX_DATA_SIZE 1024

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


void create_header(uint8_t type, 
                   char* data, 
                   uint16_t data_size, 
                   char** buffer, 
                   uint32_t conn_id, 
                   uint32_t seq_num, 
                   uint32_t ack_num
            );

uint16_t calculate_checksum(unsigned char* addr, uint32_t count);

ExitCode validate_checksum(ProtocolHeaderPtr header, int32_t size);

ExitCode check_malformed(unsigned char* buffer, int32_t received, uint8_t expected_flags, uint32_t* expected_conn_id);