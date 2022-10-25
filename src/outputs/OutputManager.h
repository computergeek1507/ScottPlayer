#ifndef OUTPUTMANAGER_H
#define OUTPUTMANAGER_H

#include "BaseOutput.h"

#include "spdlog/spdlog.h"

#include <QString>
#include <QObject>

#include <memory>
#include <vector>

class OutputManager: public QObject
{
    Q_OBJECT

public:

    OutputManager();
    bool LoadOutputs(QString const& outputConfig);

    bool OpenOutputs();
    void CloseOutputs();
    void OutputData(uint8_t* data);

Q_SIGNALS:
    void AddController(bool enabled, QString const& type, QString const& ip, QString const& channels);
    void SetChannelCount(uint64_t channels);

private:
    std::vector<std::unique_ptr<BaseOutput>> m_outputs;
    std::shared_ptr<spdlog::logger> m_logger{ nullptr };
};

#endif