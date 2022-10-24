#ifndef ARTNETOUTPUT_H
#define ARTNETOUTPUT_H

#include "IPOutput.h"

#include <QString>
#include <memory>

#define ARTNET_PACKET_HEADERLEN 18
#define ARTNET_PACKET_LEN (ARTNET_PACKET_HEADERLEN + 512)
#define ARTNET_PORT 0x1936
#define ARTNET_MAXCHANNEL 32768

struct ArtNetOutput : IPOutput
{
	ArtNetOutput();
	bool Open() override;
	void Close() override;
	void OutputFrame(uint8_t *data) override;

	uint32_t Universe{1};
	uint16_t PacketSize{510};

	uint8_t _data[ARTNET_PACKET_LEN];
	uint8_t _sequenceNum { 0 };
};

#endif