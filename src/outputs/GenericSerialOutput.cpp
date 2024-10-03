#include "GenericSerialOutput.h"

GenericSerialOutput::GenericSerialOutput()
{
    _datalen = 0;
    _data = std::vector<uint8_t>(GENERICSERIAL_MAX_CHANNELS + MAX_PREFIX_POSTFIX);
}

bool GenericSerialOutput::Open()
{
    if (!Enabled) return false;

    m_SerialPort->setPortName(IP);
    m_SerialPort->setBaudRate(BaudRate);
    //m_SerialPort->setStopBits(QSerialPort::TwoStop);

    _datalen = Channels + _prefix.size() + _postfix.size();

    if (_datalen > GENERICSERIAL_MAX_CHANNELS + MAX_PREFIX_POSTFIX) {
        return false;
    }

    if (_prefix.size() > 0) {
        memcpy(_data.data(), _prefix.data(), _prefix.size());
    }

    if (_postfix.size() > 0) {
        memcpy(_data.data() + Channels + _prefix.size(), _postfix.data(), _postfix.size());
    }

    return m_SerialPort->open(QIODevice::ReadWrite);
}

void GenericSerialOutput::Close()
{
    m_SerialPort->close();
}

void GenericSerialOutput::OutputFrame(uint8_t* data)
{
    if (!Enabled || m_SerialPort == nullptr || !m_SerialPort->isOpen()) return;

    size_t chs = std::min((size_t)Channels, (size_t)(GENERICSERIAL_MAX_CHANNELS));
    if (memcmp(&_data[_prefix.size()], &data[StartChannel - 1], chs) != 0) {
        memcpy(&_data[_prefix.size()], &data[StartChannel - 1], chs);
    }

    m_SerialPort->write((char*)&_data[0], _datalen);
}