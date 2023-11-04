#include "RenardOutput.h"

RenardOutput::RenardOutput() 
{
	_datalen = 0;
	_data = std::vector<uint8_t>(RENARD_MAX_CHANNELS + 9);
}

bool RenardOutput::Open() 
{
    if (!Enabled) return false;

    m_SerialPort->setPortName(IP);
    m_SerialPort->setBaudRate(BaudRate);
    m_SerialPort->setStopBits(QSerialPort::TwoStop);

    _datalen = Channels + 2;
    _data[0] = 0x7E;               // start of message
    _data[1] = 0x80;               // start address
    return m_SerialPort->open(QIODevice::ReadWrite);
}
void RenardOutput::Close()
{
    m_SerialPort->close();
}

void RenardOutput::OutputFrame(uint8_t* data)
{
    if (!Enabled || m_SerialPort == nullptr || m_SerialPort->isOpen()) return;
    for (int i = 0; i < Channels; i++)
    {
        SetOneChannel(i + 2, data[(StartChannel - 1) + i]);
    }

    m_SerialPort->write((char*)&_data[0], _datalen);
}

void RenardOutput::SetOneChannel(int32_t channel, unsigned char data)
{
    uint8_t RenIntensity;

    switch (data)
    {
    case 0x7D:
    case 0x7E:
        RenIntensity = 0x7C;
        break;
    case 0x7F:
        RenIntensity = 0x80;
        break;
    default:
        RenIntensity = data;
    }

    if (_data[channel + 2] != RenIntensity)
    {
        _data[channel + 2] = RenIntensity;
    }
}