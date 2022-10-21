#include "DDPOutput.h"

DDPOutput::DDPOutput()
{
	memset(_data, 0, sizeof(_data));
}

bool DDPOutput::Open()
{
	if (IP.isEmpty()) return false;
	return false; 
}

void DDPOutput::OutputFrame(uint8_t* data) 
{ 

}

void DDPOutput::Close()
{
	m_UdpSocket->close();
}