
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
        qDebug() << "Cannot bind socket: " << m_UdpSocket->errorString();
    }

    return ok;
}

void SyncManager::CloseOutputs()
{

}

void SyncManager::SendSync(uint32_t frameMS, uint32_t stepMS, QString const& fseq, QString const& media)
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
        if (!m_lastfseq.isEmpty())
        {
            SendFPPSync(m_lastfseq, 0xFFFFFFFF, 50);
        }

        if (!m_lastmedia.isEmpty())
        {
            SendFPPSync(m_lastmedia, 0xFFFFFFFF, 50);
        }

        return;
    }

    bool dosendFSEQ = false;
    bool dosendMedia = false;

    if (stepMS == 0 || stepMS == 0xFFFFFFFF) {
        dosendFSEQ = true;
        dosendMedia = true;
    }

    QFileInfo fn(fseq);
    if (fn.suffix().toLower() == "fseq")
    {
        if (m_lastfseq != fseq)
        {
            if (!m_lastfseq.isEmpty())
            {
                SendFPPSync(m_lastfseq, 0xFFFFFFFF, frameMS);
            }

            m_lastfseq = fseq;

            if (stepMS != 0)
            {
                SendFPPSync(fseq, 0, frameMS);
            }
        }

        if (!dosendFSEQ)
        {
            if (stepMS <= FPP_SEQ_SYNC_INITIAL_NUMBER_OF_FRAMES * frameMS)
            {
                // we are in the initial period
                if (stepMS - m_lastfseqmsec >= FPP_SEQ_SYNC_INTERVAL_INITIAL_FRAMES * frameMS)
                {
                    dosendFSEQ = true;
                }
            }
            else
            {
                if (stepMS - m_lastfseqmsec >= FPP_SEQ_SYNC_INTERVAL_FRAMES * frameMS)
                {
                    dosendFSEQ = true;
                }
            }
        }
    }

    if (!media.isEmpty())
    {
        if (m_lastmedia != media)
        {
            if (!m_lastmedia.isEmpty())
            {
                SendFPPSync(m_lastmedia, 0xFFFFFFFF, frameMS);
            }

            m_lastmedia = media;

            if (stepMS != 0)
            {
                SendFPPSync(media, 0, frameMS);
            }
        }

        if (!dosendMedia)
        {
            if (stepMS - m_lastmediamsec >= FPP_MEDIA_SYNC_INTERVAL_MS)
            {
                dosendMedia = true;
            }
        }
    }

    if (dosendFSEQ) {
        SendFPPSync(fseq, stepMS, frameMS);
        m_lastfseqmsec = stepMS;
    }
    if (dosendMedia) {
        SendFPPSync(media, stepMS, frameMS);
        m_lastmediamsec = stepMS;
    }

    if (stepMS == 0xFFFFFFFF)
    {
        m_lastfseq = "";
        m_lastfseqmsec = 0;
        m_lastmedia = "";
        m_lastmediamsec = 0;
    }

}

void SyncManager::SendStop()
{
    SendSync(50, 0xFFFFFFFF, "", "");
}

void SyncManager::SendFPPSync(const QString& item, uint32_t stepMS, uint32_t frameMS)
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
        sp->frameNumber = stepMS / frameMS;
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