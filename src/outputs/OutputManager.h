#ifndef OUTPUTMANAGER_H
#define OUTPUTMANAGER_H

#include "BaseOutput.h"

#include <QString>
#include <QObject>

#include <memory>
#include <vector>

class OutputManager: public QObject
{
    Q_OBJECT

public:

    OutputManager();
    void LoadOutputs(QString const& outputConfig);

    bool OpenOutputs();
    void OutputData(uint8_t* data);

Q_SIGNALS:
    void AddController(QString const& type, QString const& ip, QString const& channels);

private:
    std::vector<std::unique_ptr<BaseOutput>> m_outputs;
};

#endif