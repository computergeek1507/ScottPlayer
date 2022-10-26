
#include "SyncManager.h"

#include "SyncPacket.h"

#include <QFileInfo>

#define FPP_MEDIA_SYNC_INTERVAL_MS 500
#define FPP_SEQ_SYNC_INTERVAL_FRAMES 16
#define FPP_SEQ_SYNC_INTERVAL_INITIAL_FRAMES 4
#define FPP_SEQ_SYNC_INITIAL_NUMBER_OF_FRAMES 32

SyncManager::SyncManager(QObject* parent) :
	QObject(parent),
	m_groupAddress(QHostAddress(MULTISYNC_MULTICAST_ADDRESS)),
	m_logger(spdlog::get("scottplayer"))
{

}

SyncManager::~SyncManager()
{
	if (m_UdpSocket && m_UdpSocket->state() == QAbstractSocket::BoundState)
	{
		m_UdpSocket->close();
	}
}

bool SyncManager::OpenOutputs()
{
	if (!m_enabled)
	{
		return false;
	}
	m_UdpSocket = std::make_unique<QUdpSocket>(this);
	m_UdpSocket->setProxy(QNetworkProxy::NoProxy);
	bool ok = m_UdpSocket->bind(QHostAddress::Any, FPP_CTRL_PORT);
	if (!ok)
	{
		m_logger->error("Cannot bind socket: {}", m_UdpSocket->errorString().toStdString());
	}

	return ok;
}

void SyncManager::CloseOutputs()
{

}

void SyncManager::SendSync(uint32_t frameSizeMS, uint32_t frame, QString const& fseq, QString const& media)
{
	if (!m_enabled)
	{
		return;
	}
	if (!m_UdpSocket)
	{
		OpenOutputs();
	}
	if (fseq.isEmpty())
	{
		if (!m_lastFseq.isEmpty())
		{
			SendFPPSync(m_lastFseq, 0xFFFFFFFF, 0);
		}

		if (!m_lastMedia.isEmpty())
		{
			SendFPPSync(m_lastMedia, 0xFFFFFFFF, 0);
		}

		return;
	}

	bool dosendFSEQ{ false };
	bool dosendMedia{ false };

	if (frame == 0 || frame == 0xFFFFFFFF) {
		dosendFSEQ = true;
		dosendMedia = true;
	}

	QFileInfo fn(fseq);
	if (fn.suffix().toLower() == "fseq")
	{
		if (m_lastFseq != fseq)
		{
			if (!m_lastFseq.isEmpty())
			{
				SendFPPSync(m_lastFseq, 0xFFFFFFFF, frame);
			}

			m_lastFseq = fseq;

			if (frame != 0)
			{
				SendFPPSync(fseq, 0, frame);
			}
		}

		if (!dosendFSEQ)
		{
			if (frame <= FPP_SEQ_SYNC_INITIAL_NUMBER_OF_FRAMES)
			{
				// we are in the initial period
				if (frame - m_lastFrame >= FPP_SEQ_SYNC_INTERVAL_INITIAL_FRAMES)
				{
					dosendFSEQ = true;
				}
			}
			else
			{
				if (frame - m_lastFrame >= FPP_SEQ_SYNC_INTERVAL_FRAMES)
				{
					dosendFSEQ = true;
				}
			}
		}
	}

	if (!media.isEmpty())
	{
		if (m_lastMedia != media)
		{
			if (!m_lastMedia.isEmpty())
			{
				SendFPPSync(m_lastMedia, 0xFFFFFFFF, frame);
			}

			m_lastMedia = media;

			if (frameSizeMS != 0)
			{
				SendFPPSync(media, 0, frame);
			}
		}

		if (!dosendMedia)
		{
			if ((frame * frameSizeMS) - m_lastMediaMsec >= FPP_MEDIA_SYNC_INTERVAL_MS)
			{
				dosendMedia = true;
			}
		}
	}

	uint32_t stepMS = frame * frameSizeMS;
	if (dosendFSEQ) {
		SendFPPSync(fseq, stepMS, frame);
		m_lastFrame = frame;
	}
	if (dosendMedia) {
		SendFPPSync(media, stepMS, frame);
		m_lastMediaMsec = stepMS;
	}

	if (frame == 0xFFFFFFFF)
	{
		m_lastFseq = "";
		m_lastFrame = 0;
		m_lastMedia = "";
		m_lastMediaMsec = 0;
	}
}

void SyncManager::SendStop()
{
	SendSync(50, 0xFFFFFFFF, "", "");
}

void SyncManager::SendFPPSync(const QString& item, uint32_t stepMS, uint32_t frames)
{
	int bufsize = sizeof(ControlPkt) + sizeof(SyncPkt) + item.size();

	std::vector<uint8_t> buffer(bufsize);

	ControlPkt* cp = reinterpret_cast<ControlPkt*>(&buffer[0]);
	strncpy(cp->fppd, "FPPD", 4);
	cp->pktType = CTRL_PKT_SYNC;
	cp->extraDataLen = bufsize - sizeof(ControlPkt);

	SyncPkt* sp = reinterpret_cast<SyncPkt*>(&buffer[0] + sizeof(ControlPkt));

	if (stepMS == 0)
	{
		sp->pktType = SYNC_PKT_START;
	}
	else if (stepMS == 0xFFFFFFFF)
	{
		sp->pktType = SYNC_PKT_STOP;
	}
	else
	{
		sp->pktType = SYNC_PKT_SYNC;
	}

	QFileInfo fn(item);
	if (fn.suffix().toLower() == "fseq")
	{
		sp->fileType = SYNC_FILE_SEQ;
		sp->frameNumber = frames;
	}
	else
	{
		sp->fileType = SYNC_FILE_MEDIA;
		sp->frameNumber = 0;
	}

	if (sp->pktType == SYNC_PKT_SYNC)
	{
		sp->secondsElapsed = stepMS / 1000.0;
	}
	else
	{
		sp->frameNumber = 0;
		sp->secondsElapsed = 0;
	}

	strcpy(&sp->filename[0], fn.fileName().toLatin1());

	if (m_UdpSocket->state() == QAbstractSocket::BoundState)
	{
		m_UdpSocket->writeDatagram((char*)&buffer[0], (qint64)bufsize, m_groupAddress, FPP_CTRL_PORT);
	}
}