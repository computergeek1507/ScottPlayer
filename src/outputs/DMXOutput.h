#ifndef DMXOUTPUT_H
#define DMXOUTPUT_H

#include "SerialOutput.h"

#include <QString>

#include <memory>

#define DMX_MAX_CHANNELS 4800

struct DMXOutput : SerialOutput
{
	DMXOutput();
	bool Open() override;
	void Close() override;
	void OutputFrame(uint8_t* data) override;

	int _datalen {0};
	//std::vector<uint8_t> _data;
	uint8_t _data[DMX_MAX_CHANNELS + 6];
};

#endif