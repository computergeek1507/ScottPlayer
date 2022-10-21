#ifndef E131OUTPUT_H
#define E131OUTPUT_H

#include "IPOutput.h"

#include <QString>
#include <memory>

#define E131_PACKET_HEADERLEN 126
#define E131_PACKET_LEN (E131_PACKET_HEADERLEN + 512)
#define E131_PORT 5568
#define E131_PRIORITY 100
#define XLIGHTS_UUID "c0de0080-c69b-11e0-9572-0800200c9a66"

struct E131Output : IPOutput
{
	E131Output();
	bool Open() override;
	void Close() override;
	void OutputFrame(uint8_t *data) override;

	uint32_t Universe{1};
	uint16_t PacketSize{510};

	uint8_t _data[E131_PACKET_LEN];
	uint8_t _sequenceNum { 0 };
};

#endif