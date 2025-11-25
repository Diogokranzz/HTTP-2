# Implementation Plan - QUIC & HTTP/3

## Goal
Implement a skeletal QUIC/HTTP3 stack over UDP using IOCP.
**Note**: Since we lack OpenSSL/TLS 1.3, this will be a **Cleartext QUIC** implementation (non-standard, for educational/architectural demonstration only).

## Phase 11: QUIC (The Transport)
QUIC replaces TCP. It runs on top of UDP.
- **`sys::UdpSocket`**: Wraps `WSASocket(SOCK_DGRAM)` and `WSARecvFrom`.
- **`quic::Packet`**: Parses the QUIC header.
    - **Long Header**: Used for handshake (Initial, Handshake, Retry).
    - **Short Header**: Used for data (1-RTT).
- **`quic::Engine`**: Manages Connection IDs (CIDs) and dispatches packets to Sessions.

## Phase 12: HTTP/3 (The Application)
HTTP/3 is just HTTP semantics over QUIC streams.
- **QPACK**: Similar to HPACK but designed for out-of-order delivery.
- **Unidirectional Streams**: Control stream, QPACK encoder/decoder streams.
- **Bidirectional Streams**: Request/Response streams.

## Architecture Changes
- **`main.cpp`**: Will now have TWO listeners:
    1.  TCP Listener (Port 8080) -> HTTP/1.1 & HTTP/2
    2.  UDP Listener (Port 8080 or 4433) -> QUIC -> HTTP/3
