#ifndef PLAYLISTMANAGER_H
#define PLAYLISTMANAGER_H

#include "SequencePlayer.h"

#include "spdlog/spdlog.h"

#include <QObject>
#include <QString>

#include <memory>
#include <vector>

struct PlayList;
struct Schedule;

class PlayListManager : public QObject
{
    Q_OBJECT

public:

    PlayListManager();
    bool LoadPlayLists(QString const& configFolder);
    void SavePlayLists(QString const& configFolder);

    [[nodiscard]] std::optional<std::reference_wrapper<PlayList const>> GetPlayList(int index) const;

public Q_SLOTS:
    void UpdateStatus(QString const& sequencePath, PlaybackStatus status);

    void LoadJsonFile(const QString& jsonFile);
	void SaveJsonFile(const QString& jsonFile);
    void AddPlaylistName(QString const& playlist);
    void AddSequence(QString const& fseqPath, QString const& mediaPath, int index);

    void PlaySequence(int playlist_index, int sequence_index) const;
    void DeleteSequence(int playlist_index, int sequence_index);
    void DeletePlayList(int playlist_index);

Q_SIGNALS:
    void PlaySequenceSend(QString const& sequencePath, QString const& mediaPath) const;
    void DisplayPlaylistSend(int index);
    void AddPlaylistSend(QString const& playlist, int index);
    void MessageSend(QString const& message);

private:

    void ReadPlaylists(QJsonObject const& json);
    void ReadSchedules(QJsonObject const& json);
    void WritePlaylists(QJsonObject& json) const;
    void WriteSchedules(QJsonObject& json) const;

    std::vector<PlayList> m_playlists;
    std::vector<Schedule> m_schedules;

    std::unique_ptr<QTimer> m_scheduleTimer{nullptr};
    QThread m_scheduleThread;

    QString CurrentSequence;

    PlaybackStatus m_status{PlaybackStatus::Stopped};

    std::shared_ptr<spdlog::logger> m_logger{ nullptr };
};
#endif