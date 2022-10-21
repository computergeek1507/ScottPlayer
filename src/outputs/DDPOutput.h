#ifndef DDPOUTPUT_H
#define DDPOUTPUT_H

#include "IPOutput.h"

#include <QString>
#include <memory>

#define DDP_PACKET_HEADERLEN 10
#define DDP_PACKET_LEN (DDP_PACKET_HEADERLEN + 1440)
#define DDP_PORT 4048
#define DDP_SYNCPACKET_LEN 10
#define DDP_DISCOVERPACKET_LEN 10

#define DDP_FLAGS1_VER     0xc0   
#define DDP_FLAGS1_VER1    0x40
#define DDP_FLAGS1_TIMECODE 0x10
#define DDP_FLAGS1_PUSH    0x01
#define DDP_FLAGS1_QUERY   0x02
#define DDP_FLAGS1_REPLY   0x04
#define DDP_FLAGS1_STORAGE 0x08
#define DDP_FLAGS1_TIME    0x10

#define DDP_ID_DISPLAY       1
#define DDP_ID_CONTROL     246
#define DDP_ID_CONFIG      250
#define DDP_ID_STATUS      251
#define DDP_ID_DMXTRANSIT  254
#define DDP_ID_ALLDEVICES  255

struct DDPOutput : IPOutput
{
	DDPOutput();
	uint8_t _data[DDP_PACKET_LEN];
	uint8_t _sequenceNum{0};

	//uint8_t* _fulldata;

	bool Open() override;
	void Close() override;
	void OutputFrame(uint8_t *data) override;
	uint16_t PacketSize{1440};
	bool KeepChannels{true};
};

#endif