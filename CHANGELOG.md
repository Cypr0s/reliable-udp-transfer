# Changelog

## 1.0.0

### Implemented
- Reliable UDP transfer
- Selective repeat
- IPv4/IPv6 support
- stdin/stdout support

### Known limitations
- Fixed window size.
- Fixed retransmission timeout.
- Precise timeout may be missed due to complex while loops.