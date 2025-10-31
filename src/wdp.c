#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "wdp.h"

// Max data size
unsigned int WDP_MAX_DATA_SIZE = WDP_DEFAULT_MAX_DATA_SIZE;

// ============================================================================
// Platform assist functions
// ============================================================================

/**
 * Get error code
 */
int wdpGetLastError(void)
{
#ifdef _WIN32
    return WSAGetLastError();
#else
    return errno;
#endif
}

/**
 * Check if connection has error
 */
static int isConnectionError(int error)
{
#ifdef _WIN32
    return (error == WSAENOTCONN || error == WSAECONNRESET || error == WSAECONNABORTED);
#else
    return (error == ENOTCONN || error == ECONNRESET || error == EPIPE);
#endif
}

// ============================================================================
// Init and clean functions
// ============================================================================

/**
 * Initial wdp library
 */
int wdpInit(void)
{
#ifdef _WIN32
    WSADATA wsaData;
    int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (result != 0) {
        return WDP_ERR_SOCKET_INVALID;
    }
#endif
    return WDP_SUCCESS;
}

/**
 * Clean wdp library
 */
void wdpCleanup(void)
{
#ifdef _WIN32
    WSACleanup();
#endif
}

/**
 * Set max data size
 */
void wdpSetMaxDataSize(unsigned int maxSize)
{
    WDP_MAX_DATA_SIZE = maxSize;
}

/**
 * Get current max data size
 */
unsigned int wdpGetMaxDataSize(void)
{
    return WDP_MAX_DATA_SIZE;
}

// ============================================================================
// Public functions
// ============================================================================

WDP wdpPack(byte *data, unsigned int dataLen)
{
    WDP wdpData;
    wdpData.data = data;
    wdpData.dataLen = dataLen;
    return wdpData;
}

/**
 * Send wdp data package
 * Formate: [Version string(6 bytes)] [Data length(4 bytes，Network byte order)] [DATA]
 */
int wdpSend(wdp_socket_t socket, WDP wdp_toSend)
{
    // Parameter validation
    if (socket == WDP_INVALID_SOCKET)
        return WDP_ERR_SOCKET_INVALID;
    
    if (wdp_toSend.data == NULL && wdp_toSend.dataLen > 0)
        return WDP_ERR_PARAM_NULL_DATA;
    
    if (wdp_toSend.dataLen > WDP_MAX_DATA_SIZE)
        return WDP_ERR_PROTOCOL_DATA_TOO_LARGE;
    
    // Caculate total length
    unsigned int totalLen = WDP_HEADER_SIZE + wdp_toSend.dataLen;

    // Allocate send buffer
    byte *sendBuffer = (byte *)malloc(totalLen);
    if (sendBuffer == NULL)
        return WDP_ERR_MEMORY_ALLOC_FAILED;
    
    // Construct data package head
    byte *p = sendBuffer;
    
    // 1. Copy version string
    memcpy(p, WDP_VERSION_STRING, WDP_VERSION_LEN);
    p += WDP_VERSION_LEN;
    
    // 2. Copy data length (convert to network byte order)
    unsigned int dataLenNetwork = htonl(wdp_toSend.dataLen);
    memcpy(p, &dataLenNetwork, sizeof(unsigned int));
    p += sizeof(unsigned int);

    // 3. Copy data
    if (wdp_toSend.dataLen > 0) {
        memcpy(p, wdp_toSend.data, wdp_toSend.dataLen);
    }

    // Send data (loop to ensure completeness)
    unsigned int totalSent = 0;
    while (totalSent < totalLen) {
        int sendStatus = send(socket, 
                             (const char *)(sendBuffer + totalSent), 
                             totalLen - totalSent, 
                             0);
        
        if (sendStatus == WDP_SOCKET_ERROR) {
            int lastError = wdpGetLastError();
            free(sendBuffer);
            
            if (isConnectionError(lastError))
                return WDP_ERR_SOCKET_NOT_CONNECTED;
            else
                return WDP_ERR_NETWORK_SEND_FAILED;
        }
        
        if (sendStatus == 0) {
            free(sendBuffer);
            return WDP_ERR_NETWORK_PEER_CLOSED;
        }
        
        totalSent += sendStatus;
    }
    
    free(sendBuffer);
    return totalSent;  // Return total sent bytes
}

/**
 * Receive WDP data package (with timeout)
 */
int wdpRecv(wdp_socket_t socket, WDP *wdp_Dst, unsigned int timeoutMs)
{
    // Parameter validation
    if (socket == WDP_INVALID_SOCKET)
        return WDP_ERR_SOCKET_INVALID;
    
    if (wdp_Dst == NULL)
        return WDP_ERR_MEMORY_NULL_POINTER;
    
    // Initialize destination structure
    wdp_Dst->data = NULL;
    wdp_Dst->dataLen = 0;
    
    // 1. Receive header (version string + data length)
    byte header[WDP_HEADER_SIZE];
    int recvStatus = wdpRecvExact(socket, header, WDP_HEADER_SIZE, timeoutMs);
    
    if (recvStatus < 0)
        return recvStatus;  // Return error code
    
    if (recvStatus < WDP_HEADER_SIZE)
        return WDP_ERR_PROTOCOL_HEADER_INCOMPLETE;

    // 2. Verify version string
    if (!wdpStartsWith(header, (const byte *)WDP_VERSION_STRING, WDP_VERSION_LEN)) {
        return WDP_ERR_PROTOCOL_VERSION_MISMATCH;
    }

    // 3. Parse data length (convert from network byte order to host byte order)
    unsigned int dataLenNetwork;
    memcpy(&dataLenNetwork, header + WDP_VERSION_LEN, sizeof(unsigned int));
    unsigned int dataLen = ntohl(dataLenNetwork);

    // 4. Verify data length
    if (dataLen > WDP_MAX_DATA_SIZE)
        return WDP_ERR_PROTOCOL_DATA_TOO_LARGE;

    // 5. If data length is 0, return directly
    if (dataLen == 0) {
        wdp_Dst->dataLen = 0;
        wdp_Dst->data = NULL;
        return WDP_SUCCESS;
    }

    // 6. Allocate data buffer
    byte *dataBuffer = (byte *)malloc(dataLen);
    if (dataBuffer == NULL)
        return WDP_ERR_MEMORY_ALLOC_FAILED;

    // 7. Receive data part
    recvStatus = wdpRecvExact(socket, dataBuffer, dataLen, timeoutMs);
    
    if (recvStatus < 0) {
        free(dataBuffer);
        return recvStatus;
    }
    
    if (recvStatus < (int)dataLen) {
        free(dataBuffer);
        return WDP_ERR_NETWORK_INCOMPLETE;
    }

    // 8. Set return values
    wdp_Dst->data = dataBuffer;
    wdp_Dst->dataLen = dataLen;

    return dataLen;  // Return received data byte count
}

/**
 * Free memory allocated for WDP structure
 */
void wdpFree(WDP *wdp)
{
    if (wdp != NULL && wdp->data != NULL) {
        free(wdp->data);
        wdp->data = NULL;
        wdp->dataLen = 0;
    }
}

/**
 * Get error description string
 */
const char* wdpGetErrorString(int errorCode)
{
    switch (errorCode) {
        case WDP_SUCCESS:
            return "Success";

        // Network errors
        case WDP_ERR_NETWORK_SEND_FAILED:
            return "Network send failed";
        case WDP_ERR_NETWORK_RECV_FAILED:
            return "Network receive failed";
        case WDP_ERR_NETWORK_TIMEOUT:
            return "Network timeout";
        case WDP_ERR_NETWORK_PEER_CLOSED:
            return "Peer closed connection";
        case WDP_ERR_NETWORK_INCOMPLETE:
            return "Incomplete data received";

        // Socket errors
        case WDP_ERR_SOCKET_INVALID:
            return "Invalid socket";
        case WDP_ERR_SOCKET_NOT_CONNECTED:
            return "Socket not connected";
        case WDP_ERR_SOCKET_SELECT_FAILED:
            return "Select operation failed";

        // Memory errors
        case WDP_ERR_MEMORY_ALLOC_FAILED:
            return "Memory allocation failed";
        case WDP_ERR_MEMORY_COPY_FAILED:
            return "Memory copy failed";
        case WDP_ERR_MEMORY_NULL_POINTER:
            return "Null pointer error";

        // Protocol errors
        case WDP_ERR_PROTOCOL_VERSION_MISMATCH:
            return "Protocol version mismatch";
        case WDP_ERR_PROTOCOL_INVALID_LENGTH:
            return "Invalid data length";
        case WDP_ERR_PROTOCOL_DATA_TOO_LARGE:
            return "Data exceeds maximum size";
        case WDP_ERR_PROTOCOL_HEADER_INCOMPLETE:
            return "Incomplete header";
        
        // Parameter errors
        case WDP_ERR_PARAM_INVALID:
            return "Invalid parameter";
        case WDP_ERR_PARAM_NULL_DATA:
            return "Null data pointer";
        case WDP_ERR_PARAM_BUFFER_TOO_SMALL:
            return "Buffer too small";
        
        default:
            return "Unknown error";
    }
}

// ============================================================================
// Internal helper function implementations
// ============================================================================

/**
 * Check if a string starts with a specified prefix
 */
int wdpStartsWith(const byte *str, const byte *prefix, int prefixLen)
{
    if (str == NULL || prefix == NULL)
        return 0;
    
    return (memcmp(str, prefix, prefixLen) == 0) ? 1 : 0;
}

/**
 * Receive fixed-length data with timeout
 * Use select() for non-blocking and timeout control
 */
int wdpRecvExact(wdp_socket_t socket, byte *buffer, unsigned int length, unsigned int timeoutMs)
{
    if (buffer == NULL)
        return WDP_ERR_MEMORY_NULL_POINTER;
    
    unsigned int totalRecv = 0;
    
    while (totalRecv < length) {
        // Use select() to wait for data availability
        fd_set readfds;
        FD_ZERO(&readfds);
        FD_SET(socket, &readfds);
        
        struct timeval timeout;
        struct timeval *timeoutPtr = NULL;
        
        if (timeoutMs > 0) {
            timeout.tv_sec = timeoutMs / 1000;
            timeout.tv_usec = (timeoutMs % 1000) * 1000;
            timeoutPtr = &timeout;
        }
        
#ifdef _WIN32
        int selectResult = select(0, &readfds, NULL, NULL, timeoutPtr);
#else
        int selectResult = select(socket + 1, &readfds, NULL, NULL, timeoutPtr);
#endif
        
        if (selectResult == WDP_SOCKET_ERROR) {
            return WDP_ERR_SOCKET_SELECT_FAILED;
        }
        
        if (selectResult == 0) {
            // Timeout
            return WDP_ERR_NETWORK_TIMEOUT;
        }

        // Socket is readable, receive data
        int recvStatus = recv(socket, 
                             (char *)(buffer + totalRecv), 
                             length - totalRecv, 
                             0);
        
        if (recvStatus == WDP_SOCKET_ERROR) {
            int lastError = wdpGetLastError();
            if (isConnectionError(lastError))
                return WDP_ERR_SOCKET_NOT_CONNECTED;
            else
                return WDP_ERR_NETWORK_RECV_FAILED;
        }
        
        if (recvStatus == 0) {
            // Peer closed connection
            return WDP_ERR_NETWORK_PEER_CLOSED;
        }
        
        totalRecv += recvStatus;
    }
    
    return totalRecv;
}
