/** ------------- IPK 2 - RDT ---------------
 * @headerfile  protocol.h
 * @author      Kristian Luptak (xluptak00)
 * @date        28.4.2026
 * @brief       Contains definitions of struct of protocol, different macros, 
 *              declarations of functions
 */


#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <stdint.h> // types
#include <string.h> // memset
#include "error.h"  // exitcode

#define HEADER_SIZE 20
#define MAX_PROTOCOL_SIZE (HEADER_SIZE + MAX_DATA_SIZE)
#define MAX_DATA_SIZE 1024

// 5 hours of debugging becuse compiler was aligning differently :)
// protocol header struct
#pragma pack(push, 1)
typedef struct {
    uint32_t conn_id;
    uint32_t seq_num;
    uint32_t ack_num;
    uint16_t checksum;
    uint16_t payload_size;
    uint8_t  flags;
    uint16_t padding;
    uint8_t  padding2;
    unsigned char data[];
} ProtocolHeader, *ProtocolHeaderPtr;
#pragma pack(pop)


void create_header(uint8_t type, 
                   unsigned char* data, 
                   uint16_t data_size, 
                   unsigned char* buffer, 
                   uint32_t conn_id, 
                   uint32_t seq_num, 
                   uint32_t ack_num
            );

uint16_t calculate_checksum(unsigned char* addr, uint32_t count);

ExitCode validate_checksum(ProtocolHeaderPtr header, int32_t size);

ExitCode check_malformed(unsigned char* buffer, int32_t received);

#endif //PROTOCOL_H