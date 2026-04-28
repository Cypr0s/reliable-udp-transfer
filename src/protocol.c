#include "protocol.h"


void create_header(uint8_t type, 
                   char* data, 
                   uint16_t data_size, 
                   char** buffer, 
                   uint32_t conn_id, 
                   uint32_t seq_num, 
                   uint32_t ack_num
            ) {
    memset(*buffer, 0, HEADER_SIZE + data_size);
    ProtocolHeaderPtr header = (ProtocolHeaderPtr) *buffer;
    header->conn_id = conn_id;
    header->seq_num = seq_num;
    header->ack_num = ack_num;
    header->payload_size = data_size;
    header->flags = type;
    if(data_size > 0) {
        memcpy(header->data, data, data_size);
    }
    header->checksum = calculate_checksum((unsigned char*)header, HEADER_SIZE + data_size);
}
/**
 * taken from https://datatracker.ietf.org/doc/html/rfc1071 and modified
 */
uint16_t calculate_checksum(unsigned char* addr, uint32_t count) {
    register long sum = 0;

    while(count > 1)  {
        /*  This is the inner loop */
        sum += *(unsigned short*) addr;
        count -= 2;
        addr += 2;
    }

    /*  Add left-over byte, if any */
    if(count > 0) {
        sum += * (unsigned char *) addr;
    }
    /*  Fold 32-bit sum to 16 bits */
    while (sum>>16) {
        sum = (sum & 0xffff) + (sum >> 16);
    }

    return (uint16_t) ~sum;
}

ExitCode validate_checksum(ProtocolHeaderPtr header, int32_t size) {
    uint16_t received = header->checksum;
    header->checksum = 0;

    uint16_t calculated = calculate_checksum((unsigned char*) header, size);
    header->checksum = received;
    // malformed
    if(calculated != received) {
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

ExitCode check_malformed(unsigned char* buffer, int32_t received) {
    ProtocolHeaderPtr header = (ProtocolHeaderPtr) buffer;
    
    // not long enough
    if(received < HEADER_SIZE) {
        return EXIT_CORRUPT;
    }
    // corrupt packet
    if(validate_checksum(header, received) != EXIT_SUCCESS) {
        return EXIT_CORRUPT;
    }
    
    return EXIT_SUCCESS;
}

