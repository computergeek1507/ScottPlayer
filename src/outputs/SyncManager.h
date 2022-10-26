#ifndef SYNCMANAGER_H
#define SYNCMANAGER_H

#include "spdlog/spdlog.h"

#include <QtNetwork>
#include <QUdpSocket>
#include <QString>
#include <QObject>

#include <memory>
#include <vector>

class SyncManager : public QObject
{
	Q_OBJECT

public:

	SyncManager(QObject* parent = 0);
	~SyncManager();

	bool OpenOutputs();
	void CloseOutputs();

	void SendStop();
	void SendSync(uint32_t frameSizeMS, uint32_t frame, QString const& fseq, QString const& media);

	bool IsEnabled() const { return m_enabled; }
	void SetEnabled(bool enable)
	{
		m_enabled = enable;
	}

Q_SIGNALS:

private:
	void SendFPPSync(const QString& item, uint32_t stepMS, uint32_t frames);

	bool m_enabled{ true };
	QString m_lastFseq;
	QString m_lastMedia;
	size_t m_lastFrame{ 0 };
	size_t m_lastMediaMsec{ 0 };
	//QStringList m_unicast;
	QHostAddress m_groupAddress;
	std::unique_ptr<QUdpSocket> m_UdpSocket{ nullptr };
	std::shared_ptr<spdlog::logger> m_logger{ nullptr };
};

#endif