# WDP（Water Drop Protocol）

## 语言
- [English](README.md)
- [中文](README-cn.md)

## 描述
WDP 是一种轻量级的 TCP 数据传输协议，旨在：确保数据边界，实现完整的数据包接收（对，只有这个功能！）

## 平台支持
WDP 支持 Windows 和 Linux 平台。

## 快速开始
### 最大数据大小
出于安全考虑，WDP 会限制可接收的最大数据大小。
WDP 允许在运行时配置最大数据大小（默认 1MB）：
```c
// 获取当前data最大值
unsigned int current = wdpGetMaxDataSize();
// 将最大data值改为 2MB
wdpSetMaxDataSize(2 * 1024 * 1024);
```

### 初始化与清理
```c
#include "wdp.h"
// wdpInit 会在 Windows 上初始化 WSADATA，Linux 上无需此操作。
if (wdpInit() != 0) {
    // 处理初始化错误
}

// 在 Windows 上释放 WSADATA。
wdpCleanup();
```

### 发送数据
```c
// 使用 WDP 协议发送数据
byte dataToSend[] = "Hello, WDP!";
WDP packet = wdpPack(dataToSend, sizeof(dataToSend));
int bytesSent = wdpSend(socket, packet);
if (bytesSent < 0) {
    // 处理发送错误
}
wdpFree(&packet); // 释放分配的内存
// ...
wdpCleanup();
```

### 接收数据
```c
// 使用 WDP 协议接收数据
WDP receivedPacket;
int result = wdpRecv(socket, &receivedPacket, 5000); // 5 秒超时
if (result < 0) {
    // 处理接收错误
}
// 处理接收到的数据
receivedPacket.data; // 数据指针
receivedPacket.dataLen; // 数据长度

wdpFree(&receivedPacket); // 释放分配的内存
// ...
wdpCleanup();
```

### 简易错误处理
WDP 提供易用的错误处理函数：
```c
int errCode = wdpSend(socket, packet);
if (errCode < 0) {
    const char* errMsg = wdpGetErrorString(errCode);
    printf("WDP error: %s\n", errMsg);
}

// ====================================================

// 处理错误码
switch (errCode) {
    case WDP_ERR_NETWORK_TIMEOUT:
        // 处理超时
        break;
    case WDP_ERR_MEMORY_ALLOC_FAILED:
        // 处理内存分配失败
        break;
    // 根据需要添加更多情况
    default:
        // 处理未知错误
        break;
}
```
```c

/*
可参考 wdp.h 中定义的错误码：

// 成功
    WDP_SUCCESS = 0,

    // 网络相关错误（-100 ~ -199）
    WDP_ERR_NETWORK_SEND_FAILED = -100,      // send() 失败
    WDP_ERR_NETWORK_RECV_FAILED = -101,      // recv() 失败
    WDP_ERR_NETWORK_TIMEOUT = -102,          // 接收超时
    WDP_ERR_NETWORK_PEER_CLOSED = -103,      // 对端关闭连接
    WDP_ERR_NETWORK_INCOMPLETE = -104,       // 接收不完整

    // Socket 相关错误（-200 ~ -299）
    WDP_ERR_SOCKET_INVALID = -200,           // 无效 socket
    WDP_ERR_SOCKET_NOT_CONNECTED = -201,     // socket 未连接
    WDP_ERR_SOCKET_SELECT_FAILED = -202,     // select() 失败

    // 内存相关错误（-300 ~ -399）
    WDP_ERR_MEMORY_ALLOC_FAILED = -300,      // 内存分配失败
    WDP_ERR_MEMORY_COPY_FAILED = -301,       // memcpy 失败
    WDP_ERR_MEMORY_NULL_POINTER = -302,      // 空指针

    // 协议相关错误（-400 ~ -499）
    WDP_ERR_PROTOCOL_VERSION_MISMATCH = -400, // 版本不匹配
    WDP_ERR_PROTOCOL_INVALID_LENGTH = -401,   // 数据长度无效
    WDP_ERR_PROTOCOL_DATA_TOO_LARGE = -402,   // 数据超出最大限制
    WDP_ERR_PROTOCOL_HEADER_INCOMPLETE = -403, // 头部不完整

    // 参数相关错误（-500 ~ -599）
    WDP_ERR_PARAM_INVALID = -500,            // 参数无效
    WDP_ERR_PARAM_NULL_DATA = -501,          // 数据指针为空
    WDP_ERR_PARAM_BUFFER_TOO_SMALL = -502,   // 缓冲区太小
*/

```