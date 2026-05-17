/** ------------- Reliable UDP Transfer ---------------
 * @file        protocol.c
 * @author      Kristian Luptak (xluptak00)
 * @date        28.4.2026
 * @brief       Contains functions related to protocol like creating a header or validating cheksum
 */

#include "protocol.h"

/**
 * @brief creates a protocol header with the specified parameters based on arguments
 * @return nothing
 */
void create_header(uint8_t type, 
                   unsigned char* data, 
                   uint16_t data_size, 
                   unsigned char* buffer, 
                   uint32_t conn_id, 
                   uint32_t seq_num, 
                   uint32_t ack_num
            ) {
    // reset buffer
    memset(buffer, 0, HEADER_SIZE + data_size);
    // fill header
    ProtocolHeaderPtr header = (ProtocolHeaderPtr) buffer;
    header->conn_id = conn_id;
    header->seq_num = seq_num;
    header->ack_num = ack_num;
    header->payload_size = data_size;
    header->flags = type;
    if(data_size > 0) {
        memcpy(header->data, data, data_size);
    }
    // compute checksum
    header->checksum = calculate_checksum((unsigned char*)header, HEADER_SIZE + data_size);
} // create_header


/**
 *          taken from https://datatracker.ietf.org/doc/html/rfc1071 and modified, 
 *          also taken from my first project
 * @brief   Compute Internet Checksum for "count" bytes, beginning at location "addr
 * @return  16 bit checksum
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
} // calculate_checksum


/**
 * @brief           validates checksum received in header
 * @param   header  contains checksum
 * @param   size    size of header + data
 */
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
} // validate_checksum


/**
 * @brief               checked if received message in buffer is valid
 * @param   buffer      contains header + data of protocol
 * @param   received    size of buffer
 */
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
} // check_malformed

