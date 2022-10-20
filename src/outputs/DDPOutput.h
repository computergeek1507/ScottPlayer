#ifndef DDPOUTPUT_H
#define DDPOUTPUT_H

#include "IPOutput.h"

#include <QString>
#include <memory>

#define E131_DEFAULT_PORT     5568

struct DDPOutput : IPOutput
{
	void Open() const override { };
	void OutputFrame(uint8_t *data) const override { };
	uint16_t PacketSize{1440};
	bool KeepChannels{true};
};

#endif