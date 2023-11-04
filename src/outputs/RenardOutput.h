#ifndef RENARDOUTPUT_H
#define RENARDOUTPUT_H

#include "SerialOutput.h"

#include <QString>

#include <memory>

#define RENARD_MAX_CHANNELS 1015

struct RenardOutput : SerialOutput
{
	RenardOutput();
	bool Open() override;
	void Close() override;
	void OutputFrame(uint8_t* data) override;

	void SetOneChannel(int32_t channel, unsigned char data);

	int _datalen;
	std::vector<uint8_t> _data;
};

#endif