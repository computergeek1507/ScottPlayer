#ifndef SEQUENCEPLAYER_H
#define SEQUENCEPLAYER_H

#include "../fseq/FSEQFile.h"

#include "../outputs/OutputManager.h"

#include <QMap>
#include <QObject>
#include <QString>

#include <memory>


class SequencePlayer : public QObject
{
    Q_OBJECT
public:
    SequencePlayer() = default;

    void LoadConfigs(QString const& configPath);

public Q_SLOTS:
    void LoadSequence(QString const& sequencePath, QString const& mediaPath);
    void PlaySequence();
    void StopSequence();
    void LoadOutputs(QString const& configPath);
    void on_AddController(QString const& type, QString const& ip, QString const& channels)
    {
        emit AddController(type, ip, channels);
    }

Q_SIGNALS:
    void UpdateSequence(QString const& sequenceName, QString const& media, int frames, int frameSizeMS);
    void AddController(QString const& type, QString const& ip, QString const& channels);

private:

    FSEQFile* m_seqFile{nullptr};
    int m_seqMSDuration{0};
    int m_seqMSElapsed{0};
    int m_seqMSRemaining{0};

    int m_seqStepTime{0};
    float m_seqRefreshRate{0};

    //std::atomic_int m_lastFrameRead;
    FSEQFile::FrameData* m_lastFrameData{nullptr};

    std::unique_ptr<OutputManager> m_outputManager{nullptr};
};

#endif