#ifndef OPENDMXOUTPUT_H
#define OPENDMXOUTPUT_H

#include "SerialOutput.h"

#include <QString>

#include <memory>

#define OPENDMX_MAX_CHANNELS 512

struct OpenDMXOutput : SerialOutput
{
	OpenDMXOutput();
	bool Open() override;
	void Close() override;
	void OutputFrame(uint8_t* data) override;

	uint8_t _data[OPENDMX_MAX_CHANNELS + 1];
};

#endif