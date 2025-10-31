# WDP (Water Drop Protocol)

## Languages
- [English](README.md)
- [中文](README-cn.md)

## Description
WDP is a lightweight TCP data transfer protocol designed to: Ensure data boundaries for complete packet reception (Yeah, only this!)

## Platform Support
WDP supports both Windows and Linux platforms. 

## Quick Start
### Maximum Data Size
For security reasons, WDP will limit the maximum data size that can be received.
WDP allows configuring the maximum data size at runtime (default is 1MB):
```c
// Get current maximum data size
unsigned int current = wdpGetMaxDataSize();
// Change maximum size to 2MB
wdpSetMaxDataSize(2 * 1024 * 1024);
```

### Initialization and Cleanup
```c
#include "wdp.h"
// wdpInit will initialize WSADATA on Windows. It is not necessary on Linux.
if (wdpInit() != 0) {
    // Handle initialization error
}

// Free WSADATA on Windows.
wdpCleanup();
```

### Sending Data
```c
// Send data using WDP Protocol
byte dataToSend[] = "Hello, WDP!";
WDP packet = wdpPack(dataToSend, sizeof(dataToSend));
int bytesSent = wdpSend(socket, packet);
if (bytesSent < 0) {
    // Handle send error
}
wdpFree(&packet); // Free allocated memory
// ...
wdpCleanup();
```

### Receiving Data
```c
// Receive data using WDP Protocol
WDP receivedPacket;
int result = wdpRecv(socket, &receivedPacket, 5000); // 5 seconds timeout
if (result < 0) {
    // Handle receive error
}
// Process received data
receivedPacket.data; // Pointer to data
receivedPacket.dataLen; // Length of data

wdpFree(&receivedPacket); // Free allocated memory
// ...
wdpCleanup();
```

### Easy Error Handling
WDP provides easy-to-use error handling functions:
```c
int errCode = wdpSend(socket, packet);
if (errCode < 0) {
    const char* errMsg = wdpGetErrorString(errCode);
    printf("WDP Error: %s\n", errMsg);
}

// ====================================================

// Handle error codes
switch (errCode) {
    case WDP_ERR_NETWORK_TIMEOUT:
        // Handle timeout
        break;
    case WDP_ERR_MEMORY_ALLOC_FAILED:
        // Handle memory allocation failure
        break;
    // Add more cases as needed
    default:
        // Handle unknown error
        break;
}
```
```c

/*
You can refer to the error codes defined(in wdp.h):

// Success
    WDP_SUCCESS = 0,

    // Network-related errors (-100 ~ -199)
    WDP_ERR_NETWORK_SEND_FAILED = -100,      // send() failed
    WDP_ERR_NETWORK_RECV_FAILED = -101,      // recv() failed
    WDP_ERR_NETWORK_TIMEOUT = -102,          // Receive timeout
    WDP_ERR_NETWORK_PEER_CLOSED = -103,      // Peer closed connection
    WDP_ERR_NETWORK_INCOMPLETE = -104,       // Incomplete receive

    // Socket-related errors (-200 ~ -299)
    WDP_ERR_SOCKET_INVALID = -200,           // Invalid socket
    WDP_ERR_SOCKET_NOT_CONNECTED = -201,     // Socket not connected
    WDP_ERR_SOCKET_SELECT_FAILED = -202,     // select() failed

    // Memory-related errors (-300 ~ -399)
    WDP_ERR_MEMORY_ALLOC_FAILED = -300,      // Memory allocation failed
    WDP_ERR_MEMORY_COPY_FAILED = -301,       // memcpy failed
    WDP_ERR_MEMORY_NULL_POINTER = -302,      // Null pointer

    // Protocol-related errors (-400 ~ -499)
    WDP_ERR_PROTOCOL_VERSION_MISMATCH = -400, // Version mismatch
    WDP_ERR_PROTOCOL_INVALID_LENGTH = -401,   // Invalid data length
    WDP_ERR_PROTOCOL_DATA_TOO_LARGE = -402,   // Data exceeds maximum limit
    WDP_ERR_PROTOCOL_HEADER_INCOMPLETE = -403, // Header incomplete

    // Parameter-related errors (-500 ~ -599)
    WDP_ERR_PARAM_INVALID = -500,            // Invalid parameter
    WDP_ERR_PARAM_NULL_DATA = -501,          // Null data pointer
    WDP_ERR_PARAM_BUFFER_TOO_SMALL = -502,   // Buffer too small
*/

```
