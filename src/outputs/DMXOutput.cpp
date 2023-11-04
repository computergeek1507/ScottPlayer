#include "DMXOutput.h"

DMXOutput::DMXOutput()
{
    memset(_data, 0x00, sizeof(_data));
}

bool DMXOutput::Open()
{
    if (!Enabled) return false;

    m_SerialPort->setPortName(IP);
    m_SerialPort->setBaudRate(BaudRate);

    int len = Channels < 512 ? 513 : Channels + 1;
    _datalen = len + 5;
    _data[0] = 0x7E;               // start of message
    _data[1] = 6;                  // dmx send
    _data[2] = len & 0xFF;         // length LSB
    _data[3] = (len >> 8) & 0xFF;  // length MSB
    _data[4] = 0;                  // DMX start
    _data[_datalen - 1] = 0xE7;       // end of message
    return m_SerialPort->open(QIODevice::ReadWrite);
}
void DMXOutput::Close()
{
    m_SerialPort->close();
}

void DMXOutput::OutputFrame(uint8_t* data)
{
    if (!Enabled || m_SerialPort == nullptr || m_SerialPort->isOpen()) return;

    size_t chs = std::min((size_t)Channels, (size_t)(DMX_MAX_CHANNELS));

    if (memcmp(&_data[5], &data[StartChannel - 1], chs) == 0) {
        // nothing changed
    }
    else {
        memcpy(&_data[5], &data[StartChannel - 1], chs);
    }
    m_SerialPort->write((char*)&_data[0], _datalen);
}