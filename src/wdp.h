#ifndef WDP_H
#define WDP_H

// ============================================================================
// Platform-specific includes and definitions
// ============================================================================
#ifdef _WIN32
    // Windows platform
    #include <winsock2.h>
    #include <ws2tcpip.h>
    
    // Define socket type
    typedef SOCKET wdp_socket_t;
    #define WDP_INVALID_SOCKET INVALID_SOCKET
    #define WDP_SOCKET_ERROR SOCKET_ERROR
    
#else
    // Linux/Unix platform
    #include <sys/socket.h>
    #include <sys/select.h>
    #include <sys/types.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <unistd.h>
    #include <errno.h>

    // Define socket type (Linux uses int)
    typedef int wdp_socket_t;
    #define WDP_INVALID_SOCKET (-1)
    #define WDP_SOCKET_ERROR (-1)

    // Compatible Windows function names
    #define closesocket close
#endif

typedef unsigned char byte;

// ============================================================================
// Protocol constants definitions
// ============================================================================
#define WDP_VERSION_STRING "WDP0.1"
#define WDP_VERSION_LEN 6  // Exclude '\0'
#define WDP_HEADER_SIZE (WDP_VERSION_LEN + 4)  // Version string + 4 bytes data length
#define WDP_DEFAULT_MAX_DATA_SIZE (1024 * 1024)  // Default 1MB

// Global max data size variable (modifiable at runtime)
extern unsigned int WDP_MAX_DATA_SIZE;

// ============================================================================
// Error code definitions (negative values indicate errors)
// ============================================================================
typedef enum {
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
    
} WDP_ERROR_CODE;

// ============================================================================
// WDP data structure definition
// ============================================================================
typedef struct
{
    unsigned int dataLen;   // Data length
    byte *data;             // Data pointer (caller must manage memory)
} WDP;

// ============================================================================
// Function declarations
// ============================================================================

/**
 * Initialize WDP library (Windows requires Winsock initialization)
 * @return 0 on success, error code on failure
 */
int wdpInit(void);

/**
 * Clean up WDP library (Windows requires Winsock cleanup)
 */
void wdpCleanup(void);

/**
 * Set maximum data size
 * @param maxSize Maximum data size (in bytes)
 */
void wdpSetMaxDataSize(unsigned int maxSize);

/**
 * Get current maximum data size
 * @return Maximum data size (in bytes)
 */
unsigned int wdpGetMaxDataSize(void);

/**
 * Pack data into WDP structure
 * @param data Data pointer
 * @param dataLen Data length
 * @return WDP structure
 */
WDP wdpPack(byte *data, unsigned int dataLen);

/**
 * Send WDP packet
 * @param socket Socket
 * @param wdp_toSend WDP data to send
 * @return Number of bytes sent on success, error code on failure
 */
int wdpSend(wdp_socket_t socket, WDP wdp_toSend);

/**
 * Receive WDP packet (with timeout)
 * @param socket Socket
 * @param wdp_Dst Destination structure for received data (memory allocated internally, must be freed by caller)
 * @param timeoutMs Timeout (in milliseconds), 0 means no timeout
 * @return Number of bytes received on success, error code on failure
 */
int wdpRecv(wdp_socket_t socket, WDP *wdp_Dst, unsigned int timeoutMs);

/**
 * Free memory allocated for WDP structure
 * @param wdp WDP structure pointer to free
 */
void wdpFree(WDP *wdp);

/**
 * Get error description
 * @param errorCode Error code
 * @return Error description string
 */
const char* wdpGetErrorString(int errorCode);

/**
 * Get last system error code
 * @return Last system error code
 */
int wdpGetLastError(void);

// ============================================================================
// Internal helper functions
// ============================================================================

/**
 * Check if a string starts with a specified prefix
 * @param str String to check
 * @param prefix Prefix
 * @param prefixLen Prefix length
 * @return 1 if matched, otherwise 0
 */
int wdpStartsWith(const byte *str, const byte *prefix, int prefixLen);

/**
 * Receive fixed-length data with timeout
 * @param socket Socket
 * @param buffer Receive buffer
 * @param length Length to receive
 * @param timeoutMs Timeout (in milliseconds)
 * @return Number of bytes received on success, error code on failure
 */
int wdpRecvExact(wdp_socket_t socket, byte *buffer, unsigned int length, unsigned int timeoutMs);

#endif // WDP_H
