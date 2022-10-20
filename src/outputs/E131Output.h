#ifndef E131OUTPUT_H
#define E131OUTPUT_H

#include "IPOutput.h"

#include <QString>
#include <memory>

#define E131_DEFAULT_PORT     5568

struct E131Output : IPOutput
{
	void Open() const override { };
	void OutputFrame(uint8_t *data) const override { };
	uint32_t Universe{1};
	uint16_t PacketSize{510};
};

#endif