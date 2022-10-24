#include "ArtNetOutput.h"

#include <QString>
#include <memory>

ArtNetOutput::ArtNetOutput()
{
	memset(_data, 0, sizeof(_data));
}

bool ArtNetOutput::Open()
{
	if (IP.isEmpty() || !Enabled) return false;

    memset(_data, 0x00, sizeof(_data));
    _sequenceNum = 0;

    _data[0] = 'A';   // ID[8]
    _data[1] = 'r';
    _data[2] = 't';
    _data[3] = '-';
    _data[4] = 'N';
    _data[5] = 'e';
    _data[6] = 't';
    _data[9] = 0x50;
    _data[11] = 0x0E; // Protocol version Low
    _data[14] = (Universe & 0xFF);
    _data[15] = ((Universe & 0xFF00) >> 8);
    _data[16] = 0x02; // we are going to send all 512 bytes

    m_UdpSocket = std::make_unique<QUdpSocket>(this);


    if (IP == "MULTICAST") {
        //todo: maybe right?
        m_UdpSocket->joinMulticastGroup(QHostAddress("255.255.255.255"));
    }
    else {
        m_UdpSocket->connectToHost(IP, ARTNET_PORT);
    }

    _data[16] = (uint8_t)(PacketSize >> 8);  // Property value count (high)
    _data[17] = (uint8_t)(PacketSize & 0xff);  // Property value count (low)

    return m_UdpSocket != nullptr;
}

void ArtNetOutput::OutputFrame(uint8_t* data)
{
    if (!Enabled || m_UdpSocket == nullptr || m_UdpSocket->state() != QAbstractSocket::ConnectedState) return;
    //size_t chs = (std::min)(size, (size_t)(GetMaxChannels() - channel));

    size_t chs = PacketSize;
    if (memcmp(&_data[ARTNET_PACKET_HEADERLEN], &data[StartChannel - 1], chs) == 0) {
        // nothing changed
    }
    else {
        memcpy(&_data[ARTNET_PACKET_HEADERLEN], &data[StartChannel - 1], chs);
    }
    m_UdpSocket->write((char*)&_data, ARTNET_PACKET_LEN);
}

void ArtNetOutput::Close()
{
    m_UdpSocket->close();
}