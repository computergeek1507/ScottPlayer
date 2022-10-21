#ifndef IPOUTPUT_H
#define IPOUTPUT_H

#include "BaseOutput.h"

#include <QString>
#include <QtNetwork>

#include <memory>

struct IPOutput : BaseOutput
{
	IPOutput():m_UdpSocket(std::make_unique<QUdpSocket>()) {}
	std::unique_ptr<QUdpSocket> m_UdpSocket;
};

#endif