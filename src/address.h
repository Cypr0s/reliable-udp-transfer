/** ------------- IPK 2 - RDT ---------------
 * @headerfile  address.h
 * @author      Kristian Luptak (xluptak00)
 * @date        26.4.2026
 * @brief       Header file for address related functions, resolving addresses, ...
 */

#ifndef ADDRESS_H
#define ADDRESS_H

#include <netdb.h>  // getaddrinfo
#include "error.h"  // error codes
#include <stdlib.h> // NULL
#include <stdio.h>  // fprintf, stderr
#include "util.h"   // AppSideEnum

ExitCode resolve_address(const char* address_name, 
                        struct addrinfo** addresses, 
                        uint32_t port, 
                        AppSideEnum app_side
                    );

#endif // ADDRESS_H