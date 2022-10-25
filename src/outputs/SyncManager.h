#ifndef SYNCMANAGER_H
#define SYNCMANAGER_H

#include "spdlog/spdlog.h"

#include <QString>
#include <QObject>

#include <memory>
#include <vector>

class SyncManager: public QObject
{
    Q_OBJECT

public:

    SyncManager();
    bool LoadOutputs(QString const& outputConfig);

    bool OpenOutputs();
    void CloseOutputs();
    void OutputData(QString const& fseq, QString const& media, int frame, int ms );

Q_SIGNALS:

private:
    QStringList m_unicast;
    std::shared_ptr<spdlog::logger> m_logger{ nullptr };
};

#endif