# IPK project 2 - Reliable File Transfer over UDP

## Overview

This project implements simple reliable transfer over UDP protocol. 
Protocol safely transfers files despite packet loss, duplicates, corruption and reordering.
The protocol is inspired by TCP heavily, essentialy making it simpler TCP and it uses selective repeat principle for sending the data.
My main implementation goal was that not using heap allocation at all, so project only uses *static allocation* and *stack*.


## Build

Program can be compiled by running command `make` inside the root of the repository.
```bash
make
```

To remove everything created by `make` run `make clean`.
```bash
make clean
```

## Usage - copied and edited from IPK-2 README

Project can be run as both client and server. 
User must use atleast one type and the meaning of arguments will differ for different type.

**Server**
```bash
./ipk-rdt -s -p PORT [-a ADDRESS] [-o OUTPUT] [-w TIMEOUT] [-h | --help]
```

**Client**
```bash
./ipk-rdt -c -a HOST -p PORT [-i INPUT] [-w TIMEOUT] [-h | --help]
```

### Arguments
| Argument | Description |
|----------|-------------|
| `-h` or `--help` | Writes usage instructions to stdout and exits with code 0. |
| `-s` | Starts the receiving (server) side of the application. |
| `-c` | Starts the sending (client) side of the application. |
| `-p PORT` | Specifies the UDP port number. |
| `-a ADDRESS` (server mode) | Local bind address. If omitted, listens on all local addresses. |
| `-a HOST` (client mode) | Destination hostname or IPv4/IPv6 address. If multiple addresses resolve, at least one must be tried. |
| `-i INPUT` | Input file to send. If omitted or `-`, reads from stdin. |
| `-o OUTPUT` | Output file to write received data. If omitted or `-`, writes to stdout. |
| `-w TIMEOUT` | Timeout in seconds (positive integer). Default is `1`. |

**Required arguments**
Atleast one of `-c` or `s`, port `p`, address `-a` for client only.

`Arguments may be in any order`

### Examples

```bash
# File to file transfer
./ipk-rdt -s -p 9000 -o received.bin
./ipk-rdt -c -a 127.0.0.1 -p 9000 -i sample.bin
```
```bash
# Stdin to Stdout transfer
./ipk-rdt -s -p 9000
printf 'IPK\n' | ./ipk-rdt -c -a 127.0.0.1 -p 9000
```

## Header format
Header format was taken from the TCP header format and simplified for this assignment.
Header format is specified as:

<pre>
 <--------------------32 bits------------------------->

 |----------------------------------------------------|
 |               connection id                        |
 |----------------------------------------------------|
 |               sequence number                      |
 |----------------------------------------------------|
 |              acknowledgment number                 |
 |----------------------------------------------------|
 |    checksum-16bits    |       payload_size-16bits  |
 |----------------------------------------------------|
 | flags-8bits  |          padding-24bits             |
 |----------------------------------------------------|
</pre>
Where:
- **connection id**: Connection between client and sever, in this program only one connection may happen, used for checking malformed headers.
- **sequence number**: Sequence number of the current packet, used for ordering.
- **acknowledgment number**: Used for acknowleding messages during hashake or teardown or in data transfer.
- **checksum (16 bits)**: Used for corruption detection.
- **payload_size (16 bits)**: Size of the payload in bytes, maximum 1024.
- **flags (8 bits)**: Control flags for managing the connection and packet state taken from TCP - SYN, ACK, RST, FIN, DATA.
- **padding (24 bits)**: Alignemt to 20 bits.

## Session Establishment and termination

### Client

- **Establishemnt**: Client firstly generates random connection number and starting sequence number, and sets timeout for recv function in socket.
                    After that client sends SYN packet with filled connect.ion number and sequence number and waits for response SYN + ACK. If no response comes in recv timeout, client retransmits SYN packet until SYN + ACK. After SYN + ACK number with correct acknowledgment number comes, client sends ACK response and moves onto data transfer.
- **Termination**: programmed EXACTLY like establishemnt but instead of SYN client sends FIN and waits for FIN + ACK.
**Server**
