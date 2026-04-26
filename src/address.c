/** ------------- IPK 2 - RDT ---------------
 * @file    address.c
 * @author  Kristian Luptak (xluptak00)
 * @date    26.4.2026
 * @brief   Implements function related to addresses, resolving them, ...
 */

#include "address.h"

/**
 * @brief   resolves address name into struct addrinfo, handles both client and server, errors, 
 *          caller must free addresses after use
 * @param   address_name - addres name to be resolved
 * @param   addresses - pointer to struct addrinfo* which is returned
 * @param   port - port number
 * @param   app_side - server/client
 * @return  EXIT_SUCCESS(0) - success
 *          EXIT_PARSE(2) - parsing error EAI_NONAME err - invalid address name
 *          EXIT_GETADDRINFO(4) - getaddrinfo errors
 */
ExitCode resolve_address(const char* address_name, 
                        struct addrinfo** addresses, 
                        uint32_t port, 
                        AppSideEnum app_side
                    ) {
    struct addrinfo hints = {0};

    // Convert int to string
    char port_str[6];
    snprintf(port_str, sizeof(port_str), "%d", port);

    // both ipv4 and ipv6 for udp
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_protocol = 0;
    hints.ai_flags = app_side == SERVER ? AI_PASSIVE : 0; // all addreses for server, if address_name is NULL
    
    // actually resolve address
    int32_t result = getaddrinfo(address_name, port_str, &hints, addresses);
    if (result != 0 || addresses == NULL || *addresses == NULL || (*addresses)->ai_addr == NULL) {
        if(result == EAI_NONAME) {
            fprintf(stderr, "Invalid address name\n");
            return EXIT_PARSE;
        }
        else {
            fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(result));
            return EXIT_GETADDRINFO;
        }
    }

    return EXIT_SUCCESS;
} // resolve_address
