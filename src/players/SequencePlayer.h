#ifndef SEQUENCEPLAYER_H
#define SEQUENCEPLAYER_H

#include "../fseq/FSEQFile.h"

#include "../outputs/OutputManager.h"
#include "../outputs/SyncManager.h"

#include "spdlog/spdlog.h"

#include <QMap>
#include <QObject>
#include <QString>
#include <QTimer>
#include <QThread>

#include <QMediaPlayer>

#include <memory>
#include <chrono>

#define FPPD_MAX_CHANNELS (8192 * 1024)
#define DATA_DUMP_SIZE 28

enum class SeqType
{
    Animation,
    Music
};

enum class PlaybackStatus
{
    Playing,
    Loading,
    Stopped
};

class SequencePlayer : public QObject
{
    Q_OBJECT
public:
    SequencePlayer();
    ~SequencePlayer();

    void LoadConfigs(QString const& configPath);

public Q_SLOTS:
    void LoadSequence(QString const& sequencePath, QString const& mediaPath = QString());

    void StopSequence();
    void LoadOutputs(QString const& configPath);
    void LoadSync(QString const& configPath);
    void on_AddController(bool enabled, QString const& type, QString const& ip, QString const& channels)
    {
        emit AddController(enabled, type, ip, channels);
    }
    void setTotalChannels(uint64_t channels)
    {
        channelsCount = channels;
    }
    void MediaStatusChanged(QMediaPlayer::MediaStatus status);
    void TriggerOutputData();
    void TriggerTimedOutputData(qint64 timeMS);
   

Q_SIGNALS:
    void UpdateSequence(QString const& sequenceName, QString const& media, int frames, int frameSizeMS);
    void AddController(bool enabled, QString const& type, QString const& ip, QString const& channels);
    void UpdateStatus(QString const& message);
    void UpdateTime(QString const& sequenceName, int elapsedMS, int durationMS);

    void UpdatePlaybackStatus(QString const& sequencePath, PlaybackStatus status);

private:
    void PlaySequence();
    bool LoadSeqFile(QString const& sequencePath);
    void StartAnimationSeq();

    void StartMusicSeq();

    QString m_seqFileName;
    QString m_mediaFile;
    FSEQFile* m_seqFile{nullptr};
    //std::chrono::time_point<std::chrono::high_resolution_clock> m_seqMSElapsed;
    int m_seqMSDuration{0};
    //int m_seqMSElapsed{0};
    //int m_seqMSRemaining{0};
    int m_lastFrameRead{0};
    int m_numberofFrame{0};

    int m_seqStepTime{0};
    //float m_seqRefreshRate{0};
    uint64_t channelsCount{0};

    std::unique_ptr<QTimer> m_playbackTimer{nullptr};
    QThread m_playbackThread;

    SeqType m_seqType { SeqType::Animation };

    //std::atomic_int m_lastFrameRead;
    FSEQFile::FrameData* m_lastFrameData{nullptr};

    std::unique_ptr<QMediaPlayer> m_mediaPlayer{nullptr};

    std::unique_ptr<OutputManager> m_outputManager{nullptr};

    std::unique_ptr<SyncManager> m_syncManager{nullptr};

    std::shared_ptr<spdlog::logger> m_logger{ nullptr };

    char m_seqData[FPPD_MAX_CHANNELS];
};

#endif