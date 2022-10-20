#ifndef IPOUTPUT_H
#define IPOUTPUT_H

#include "BaseOutput.h"

#include <QString>
#include <QtNetwork>

struct IPOutput : BaseOutput
{
	
	QSharedPointer<QUdpSocket> m_UdpSocket;
};

#endif