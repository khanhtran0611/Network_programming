// Ví dụ nội dung file shared/protocol.h
#ifndef PROTOCOL_H
#define PROTOCOL_H

#define SERVER_PORT 8080

// Mã lệnh từ Client -> Server
#define REQ_REGISTER "REG"
#define REQ_LOGIN "LOG"
#define REQ_GET_GROUPS "GGR"
// ...

// Mã phản hồi từ Server -> Client
#define RES_OK "OK"
#define RES_ERROR "ERR"

#endif