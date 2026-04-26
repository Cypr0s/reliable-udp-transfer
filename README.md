#Designed header built on TOP of udp
32 bits seq, ack, connection id, checksum
8 for flags - decided to use SYN, ACK, DATA, RST, FIN
16 for payload length
padding would be implicit in C struct so why not just add it
so 192 bits for header = 24 bytes
1024 BYTES for payload MAX
looks like this
\-------------
32 con_id   |  32 seq      |    32 ack     |     32 checksum    |  16   payload   |     8   flags    |   8 padding  | payload 1024 bytes MAX

Algorithm chosen Selective repeat
