#ifndef SYNCPACKET_H
#define SYNCPACKET_H

#include <cstdint>

// 239.F.P.P
#define MULTISYNC_MULTICAST_ADDRESS "239.70.80.80"
#define FPP_CTRL_PORT 32320

#define CTRL_PKT_SYNC 1
#define CTRL_PKT_BLANK 3
#define CTRL_PKT_PING 4
#define CTRL_PKT_FPPCOMMAND 6

#pragma pack(push, 1)
struct ControlPkt
{
    char fppd[4];          // 'FPPD'
    uint8_t pktType;       // Control Packet Type
    uint16_t extraDataLen; // Length of extra data following Control Packet
};
#pragma pack(pop)

#define SYNC_PKT_START 0
#define SYNC_PKT_STOP 1
#define SYNC_PKT_SYNC 2
#define SYNC_PKT_OPEN 3

#define SYNC_FILE_SEQ 0
#define SYNC_FILE_MEDIA 1

#pragma pack(push, 1)
struct SyncPkt
 {
    uint8_t pktType;      // Sync Packet Type
    uint8_t fileType;     // File Type being synced
    uint32_t frameNumber; // Sequence frames displayed on Master
    float secondsElapsed; // Seconds elapsed in file on Master
    char filename[1];     // Null-terminated File Name to play on Slave
                          // (data may continue past this header)
};
#pragma pack(pop)

#endif