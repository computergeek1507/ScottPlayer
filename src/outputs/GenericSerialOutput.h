#ifndef GENERICSERIALOUTPUT_H
#define GENERICSERIALOUTPUT_H

#include "SerialOutput.h"

#define GENERICSERIAL_MAX_CHANNELS 8192
#define MAX_PREFIX_POSTFIX 256

struct GenericSerialOutput : SerialOutput
{
	GenericSerialOutput();
	bool Open() override;
	void Close() override;
	void OutputFrame(uint8_t* data) override;

	int _datalen;
	std::vector<uint8_t> _data;
	std::vector<uint8_t> _prefix;
	std::vector<uint8_t> _postfix;
};

#endif