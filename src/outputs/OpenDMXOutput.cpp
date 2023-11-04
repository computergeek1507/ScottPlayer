#include "OpenDMXOutput.h"

OpenDMXOutput::OpenDMXOutput()
{
    memset(_data, 0x00, sizeof(_data));
}

bool OpenDMXOutput::Open()
{
    if (!Enabled) return false;

    m_SerialPort->setPortName(IP);
    m_SerialPort->setBaudRate(BaudRate);
    m_SerialPort->setStopBits(QSerialPort::TwoStop);
    return m_SerialPort->open(QIODevice::ReadWrite);
}

void OpenDMXOutput::Close()
{
    m_SerialPort->close();
}

void OpenDMXOutput::OutputFrame(uint8_t* data)
{
    if (!Enabled || m_SerialPort == nullptr || m_SerialPort->isOpen()) return;

    size_t chs = std::min(Channels, (size_t)(OPENDMX_MAX_CHANNELS));
    if (memcmp(&_data[0], &data[StartChannel - 1], chs) != 0) {
        memcpy(&_data[0], &data[StartChannel - 1], chs);
    }

    m_SerialPort->write((char*)&_data[0], chs);
}