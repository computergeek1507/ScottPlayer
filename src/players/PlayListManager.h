#ifndef PLAYLISTMANAGER_H
#define PLAYLISTMANAGER_H

#include "SequencePlayer.h"

#include "Schedule.h"

#include "spdlog/spdlog.h"

#include <QObject>
#include <QString>

#include <memory>
#include <optional>
#include <vector>

struct PlayList;
//struct Schedule;

class PlayListManager : public QObject
{
    Q_OBJECT

public:

    PlayListManager();
    ~PlayListManager();
    bool LoadPlayLists(QString const& configFolder);
    void SavePlayLists(QString const& configFolder);

    [[nodiscard]] std::optional<std::reference_wrapper<PlayList const>> GetPlayList(int index) const;
    [[nodiscard]] std::optional<std::reference_wrapper<PlayList const>> GetPlayList(QString const& name) const;
    [[nodiscard]] QStringList GetPlayLists() const;
    [[nodiscard]] std::vector<Schedule> const& GetSchedules() const { return m_schedules; };
    [[nodiscard]] bool HasPlaylists() const { return !m_playlists.empty(); };
    [[nodiscard]] bool HasSchedules() const { return !m_schedules.empty(); };

public Q_SLOTS:
    void UpdateStatus(QString const& sequencePath, PlaybackStatus status);

    void LoadJsonFile(const QString& jsonFile);
	void SaveJsonFile(const QString& jsonFile);
    void AddPlaylistName(QString const& playlist);
    void AddSequence(QString const& fseqPath, QString const& mediaPath, int index);

    void PlaySequence(int playlist_index, int sequence_index) const;
    void DeleteSequence(int playlist_index, int sequence_index);
    void DeletePlayList(int playlist_index);
    void MoveSequenceUp(int playlist_index, int sequence_index);
    void MoveSequenceDown(int playlist_index, int sequence_index);

    void AddSchedule(Schedule schedule);
    void EditSchedule(int schedule_index, Schedule schedule);

    void DeleteSchedule(int schedule_index);
    void MoveScheduleUp(int schedule_index);
    void MoveScheduleDown(int schedule_index);

    void CheckSchedule();

Q_SIGNALS:
    void PlaySequenceSend(QString const& sequencePath, QString const& mediaPath) const;
    void DisplayPlaylistSend(int index);
    void AddPlaylistSend(QString const& playlist, int index);
    void MessageSend(QString const& message);
    void SelectSequenceSend(int index);
    void DisplayScheduleSend();

private:

    void ReadPlaylists(QJsonObject const& json);
    void ReadSchedules(QJsonObject const& json);
    void WritePlaylists(QJsonObject& json) const;
    void WriteSchedules(QJsonObject& json) const;

    void PlayNextSequence();
    void PlayNewPlaylist(QString const& playlistName);

    std::vector<PlayList> m_playlists;
    std::vector<Schedule> m_schedules;

    std::unique_ptr<QTimer> m_scheduleTimer{nullptr};
    std::unique_ptr<QThread> m_scheduleThread;

    QString m_currentPlaylist;
    //QString m_currentSequence;
    int m_nextSequenceIdx{0};

    PlaybackStatus m_status{PlaybackStatus::Stopped};

    std::shared_ptr<spdlog::logger> m_logger{ nullptr };
};
#endif
