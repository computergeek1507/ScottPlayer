#include "E131Output.h"

#include <QString>
#include <memory>

E131Output::E131Output()
{
	memset(_data, 0, sizeof(_data));
}

std::string GetTag()
{
    // creates a unique tag per running instance of xLights on this machine
    return "xLights " + QString::number(QCoreApplication::applicationPid()).toStdString();
}

bool E131Output::Open()
{
	if (IP.isEmpty() || !Enabled) return false;

    memset(_data, 0x00, sizeof(_data));
    _sequenceNum = 0;
    uint8_t UnivHi = Universe >> 8;   // Universe Number (high)
    uint8_t UnivLo = Universe & 0xff; // Universe Number (low)

    _data[1] = 0x10;   // RLP preamble size (low)
    _data[4] = 0x41;   // ACN Packet Identifier (12 bytes)
    _data[5] = 0x53;
    _data[6] = 0x43;
    _data[7] = 0x2d;
    _data[8] = 0x45;
    _data[9] = 0x31;
    _data[10] = 0x2e;
    _data[11] = 0x31;
    _data[12] = 0x37;
    _data[16] = 0x72;  // RLP Protocol flags and length (high)
    _data[17] = 0x6e;  // 0x26e = 638 - 16
    _data[21] = 0x04;

    // CID/UUID

    QString id = XLIGHTS_UUID;
    id.replace("-", "");
    id = id.toLower();
    if (id.length() != 32) throw "invalid CID";
    for (int i = 0, j = 22; i < 32; i += 2) {
        wchar_t msb = id[i].toLatin1();
        wchar_t lsb = id[i + 1].toLatin1();
        msb -= isdigit(msb) ? 0x30 : 0x57;
        lsb -= isdigit(lsb) ? 0x30 : 0x57;
        _data[j++] = (uint8_t)((msb << 4) | lsb);
    }

    _data[38] = 0x72;  // Framing Protocol flags and length (high)
    _data[39] = 0x58;  // 0x258 = 638 - 38
    _data[43] = 0x02;
    // Source Name (64 bytes)
    strcpy((char*)&_data[44], GetTag().c_str());
    _data[108] = E131_PRIORITY;  // Priority (Default is 100)
    _data[113] = UnivHi;  // Universe Number (high)
    _data[114] = UnivLo;  // Universe Number (low)
    _data[115] = 0x72;  // DMP Protocol flags and length (high)
    _data[116] = 0x0b;  // 0x20b = 638 - 115
    _data[117] = 0x02;  // DMP Vector (Identifies DMP Set Property Message PDU)
    _data[118] = 0xa1;  // DMP Address Type & Data Type
    _data[122] = 0x01;  // Address Increment (low)
    _data[123] = 0x02;  // Property value count (high)
    _data[124] = 0x01;  // Property value count (low)

    m_UdpSocket = std::make_unique<QUdpSocket>(this);


    if (IP.startsWith("239.255.") || IP == "MULTICAST") {
        // multicast - universe number must be in lower 2 bytes
        QString ipaddrWithUniv = QString("239.255.%1.%2").arg((int)UnivHi).arg((int)UnivLo);
        m_UdpSocket->joinMulticastGroup(QHostAddress(ipaddrWithUniv));
    }
    else {
        m_UdpSocket->connectToHost(IP, E131_PORT);
    }

    uint8_t NumHi = (PacketSize + 1) >> 8;   // Channels (high)
    uint8_t NumLo = (PacketSize + 1) & 0xff; // Channels (low)

    _data[123] = NumHi;  // Property value count (high)
    _data[124] = NumLo;  // Property value count (low)

    int i = E131_PACKET_LEN - 16 - (512 - PacketSize);
    uint8_t hi = i >> 8;   // (high)
    uint8_t lo = i & 0xff; // (low)

    _data[16] = hi + 0x70;  // RLP Protocol flags and length (high)
    _data[17] = lo;  // 0x26e = E131_PACKET_LEN - 16

    i = E131_PACKET_LEN - 38 - (512 - PacketSize);
    hi = i >> 8;   // (high)
    lo = i & 0xff; // (low)
    _data[38] = hi + 0x70;  // Framing Protocol flags and length (high)
    _data[39] = lo;  // 0x258 = E131_PACKET_LEN - 38

    i = E131_PACKET_LEN - 115 - (512 - PacketSize);
    hi = i >> 8;   // (high)
    lo = i & 0xff; // (low)
    _data[115] = hi + 0x70;  // DMP Protocol flags and length (high)
    _data[116] = lo;  // 0x20b = E131_PACKET_LEN - 115

    return m_UdpSocket != nullptr;
}

void E131Output::OutputFrame(uint8_t* data)
{
    if (!Enabled || m_UdpSocket == nullptr || m_UdpSocket->state() != QAbstractSocket::ConnectedState) return;
    //size_t chs = (std::min)(size, (size_t)(GetMaxChannels() - channel));

    size_t chs = PacketSize;
    if (memcmp(&_data[E131_PACKET_HEADERLEN], &data[StartChannel - 1], chs) == 0) {
        // nothing changed
    }
    else {
        memcpy(&_data[E131_PACKET_HEADERLEN], &data[StartChannel - 1], chs);
    }
    m_UdpSocket->write((char*)&_data, E131_PACKET_LEN);
}

void E131Output::Close()
{
    m_UdpSocket->close();
}