#include "DDPOutput.h"

DDPOutput::DDPOutput()
{
	memset(_data, 0, sizeof(_data));
}

bool DDPOutput::Open()
{
	if (IP.isEmpty() || !Enabled) return false;

	//if (_fulldata != nullptr) delete _fulldata;
    //_fulldata = (uint8_t*)malloc(Channels);
    //if (_fulldata == nullptr) {
    //    
    //    return false;
    //}

	memset(_data, 0x00, sizeof(_data));

    _data[2] = 1;
    _data[3] = DDP_ID_DISPLAY;
    _sequenceNum = 1;

    m_UdpSocket->connectToHost(IP, DDP_PORT);

    return m_UdpSocket != nullptr;
}

void DDPOutput::OutputFrame(uint8_t* data) 
{ 
    if (!Enabled || m_UdpSocket == nullptr || m_UdpSocket->state() != QAbstractSocket::ConnectedState) return;

    int32_t index = StartChannel;
    int32_t chan = KeepChannels ? (StartChannel - 1) : 0;
    int32_t tosend = Channels;

    while (tosend > 0)
    {
        int32_t thissend = (tosend < PacketSize) ? tosend : PacketSize;

        if (tosend == thissend) {
            _data[0] = DDP_FLAGS1_VER1 | DDP_FLAGS1_PUSH;
        }
        else {
            _data[0] = DDP_FLAGS1_VER1;
        }

        _data[1] = (_data[1] & 0xF0) + _sequenceNum;

        _data[4] = (chan & 0xFF000000) >> 24;
        _data[5] = (chan & 0xFF0000) >> 16;
        _data[6] = (chan & 0xFF00) >> 8;
        _data[7] = (chan & 0xFF);

        _data[8] = (thissend & 0xFF00) >> 8;
        _data[9] = thissend & 0x00FF;

        memcpy(&_data[10], data + index, thissend);

        m_UdpSocket->write((char*)&_data[0], DDP_PACKET_LEN - (1440 - thissend));
        _sequenceNum = _sequenceNum == 15 ? 1 : _sequenceNum + 1;

        tosend -= thissend;
        index += thissend;
        chan += thissend;
    }
}

void DDPOutput::Close()
{
	m_UdpSocket->close();
}