#include "PlayListManager.h"

#include "PlayList.h"

#include <QFile>
#include <QDir>
#include <QTextStream>
#include <QJsonObject>
#include <QJsonDocument>
#include <QJsonArray>

PlayListManager::PlayListManager():
	m_scheduleTimer(std::make_unique<QTimer>(this)),
		m_logger(spdlog::get("scottplayer"))
{
	m_scheduleTimer->setInterval(2000);
	m_scheduleTimer->moveToThread(&m_scheduleThread);

	connect(&m_scheduleThread, SIGNAL(started()), m_scheduleTimer.get(), SLOT(start()));
	connect(m_scheduleTimer.get(), SIGNAL(timeout()), this, SLOT(CheckSchedule()));
	connect(this, SIGNAL(finished()), m_scheduleTimer.get(), SLOT(stop()));
	connect(this, SIGNAL(finished()), &m_scheduleThread, SLOT(quit()));
	m_scheduleThread.start();
}

PlayListManager::~PlayListManager()
{
	m_scheduleTimer->stop();
	m_scheduleThread.requestInterruption();
	m_scheduleThread.quit();
	m_scheduleThread.wait();
}

bool PlayListManager::LoadPlayLists(QString const& configFolder)
{
	QString const filepath = configFolder + QDir::separator() + "scottplayer.json";
	if(!QFile::exists(configFolder))
	{
		m_logger->warn("config file not found: {}", configFolder.toStdString());
		return false;
	}
	LoadJsonFile(filepath);
	return true;
}

void PlayListManager::SavePlayLists(QString const& configFolder)
{
	QString const filepath = configFolder + QDir::separator() + "scottplayer.json";
	SaveJsonFile(filepath);
	MessageSend("Saved: scottplayer.json" );
}

void PlayListManager::PlaySequence(int playlist_index, int sequence_index) const
{
	if (playlist_index < 0 || playlist_index > m_playlists.size())
	{
		return;
	}

	if (sequence_index < 0 || sequence_index > m_playlists.at(playlist_index).PlayListItems.size())
	{
		return;
	}

	PlaySequenceSend(m_playlists.at(playlist_index).PlayListItems.at(sequence_index).SequenceFile,
		m_playlists.at(playlist_index).PlayListItems.at(sequence_index).MediaFile);
}

void PlayListManager::DeleteSequence(int playlist_index, int sequence_index)
{
	if (playlist_index < 0 || playlist_index > m_playlists.size())
	{
		return;
	}

	if (sequence_index < 0 || sequence_index > m_playlists.at(playlist_index).PlayListItems.size())
	{
		return;
	}

	m_playlists.at(playlist_index).PlayListItems.erase(m_playlists.at(playlist_index).PlayListItems.begin() +sequence_index);
	emit DisplayPlaylistSend(playlist_index);
}

void PlayListManager::MoveSequenceUp(int playlist_index, int sequence_index)
{
	if (playlist_index < 0 || playlist_index > m_playlists.size())
	{
		return;
	}

	if (sequence_index <= 0 || sequence_index > m_playlists.at(playlist_index).PlayListItems.size())
	{
		return;
	}

	std::swap(m_playlists.at(playlist_index).PlayListItems.at(sequence_index),
		m_playlists.at(playlist_index).PlayListItems.at(sequence_index - 1));

	emit DisplayPlaylistSend(playlist_index);
	emit SelectSequenceSend(sequence_index - 1);
}
void PlayListManager::MoveSequenceDown(int playlist_index, int sequence_index) 
{
	if (playlist_index < 0 || playlist_index > m_playlists.size())
	{
		return;
	}

	if (sequence_index < 0 || sequence_index + 1 >= m_playlists.at(playlist_index).PlayListItems.size())
	{
		return;
	}

	std::swap(m_playlists.at(playlist_index).PlayListItems.at( sequence_index),
		m_playlists.at(playlist_index).PlayListItems.at(sequence_index + 1));
	
	emit DisplayPlaylistSend(playlist_index);
	emit SelectSequenceSend(sequence_index + 1);
}

void PlayListManager::DeletePlayList(int playlist_index)
{
	if (playlist_index < 0 || playlist_index > m_playlists.size())
	{
		return;
	}	

	m_playlists.erase(m_playlists.begin() + playlist_index);

	//redraw
}

void PlayListManager::UpdateStatus(QString const& sequencePath, PlaybackStatus status)
{
	m_status = status;
}

void PlayListManager::AddPlaylistName(QString const& playlist)
{
	if (std::any_of(m_playlists.begin(), m_playlists.end(), [&](auto const& elem)
		{ return elem.Name == playlist; })) {
		m_logger->warn("Cannot have Duplicate PlayList Names: {}", playlist.toStdString());
		return;
	}
	m_playlists.emplace_back(playlist);
	emit AddPlaylistSend(m_playlists.back().Name, m_playlists.size() - 1 );
}

void PlayListManager::AddSequence(QString const& fseqPath, QString const& mediaPath, int index)
{
	if (index < 0 || index > m_playlists.size())
	{
		return;
	}
	m_playlists.at(index).PlayListItems.emplace_back(fseqPath, mediaPath);
	emit DisplayPlaylistSend(index);
}

void PlayListManager::AddSchedule(Schedule schedule)
{
	m_schedules.emplace_back(std::move(schedule));
	emit DisplayScheduleSend();
}

void PlayListManager::DeleteSchedule(int schedule_index) 
{
	if (schedule_index < 0 || schedule_index > m_schedules.size())
	{
		return;
	}

	m_schedules.erase(m_schedules.begin() + schedule_index);
	emit DisplayScheduleSend();
}

void PlayListManager::EditSchedule(int schedule_index, Schedule schedule)
{
	if (schedule_index < 0 || schedule_index > m_schedules.size())
	{
		return;
	}
	m_schedules[schedule_index] = std::move(schedule);
	emit DisplayScheduleSend();
}

void PlayListManager::MoveScheduleUp(int schedule_index)
{
	if (schedule_index < 0 || schedule_index + 1 >= m_schedules.size())
	{
		return;
	}
	std::swap(m_schedules.at(schedule_index),
		m_schedules.at(schedule_index + 1));
	emit DisplayScheduleSend();
}

void PlayListManager::MoveScheduleDown(int schedule_index) 
{
	if (schedule_index <= 0 || schedule_index > m_schedules.size())
	{
		return;
	}

	std::swap(m_schedules.at(schedule_index),
		m_schedules.at(schedule_index - 1));
	emit DisplayScheduleSend();
	
}

void PlayListManager::LoadJsonFile(const QString& jsonFile)
{
	QFile loadFile(jsonFile);
	if (!loadFile.open(QIODevice::ReadOnly))
	{
		return;
	}

	QByteArray saveData = loadFile.readAll();

	QJsonDocument loadDoc(QJsonDocument::fromJson(saveData));

	ReadPlaylists(loadDoc.object());
	ReadSchedules(loadDoc.object());
}

void PlayListManager::SaveJsonFile(const QString& jsonFile)
{
	QFile saveFile(jsonFile);
	if (!saveFile.open(QIODevice::WriteOnly))
	{
		return;
	}

	QJsonObject projectObject;
	WritePlaylists(projectObject);
	WriteSchedules(projectObject);
	QJsonDocument saveDoc(projectObject);
	saveFile.write(saveDoc.toJson());
}

void PlayListManager::ReadPlaylists(QJsonObject const& json)
{
	m_playlists.clear();
	QJsonArray playlistArray = json["playlists"].toArray();
	for (auto const& playlist : playlistArray)
	{
		QJsonObject playlistObj = playlist.toObject();
		m_playlists.emplace_back(playlistObj);
		emit AddPlaylistSend(m_playlists.back().Name, m_playlists.size() - 1 );
	}
}

void PlayListManager::ReadSchedules(QJsonObject const& json)
{
	m_schedules.clear();
	QJsonArray scheduleArray = json["schedules"].toArray();
	for (auto const& schedule :scheduleArray)
	{
		QJsonObject scheduleObj = schedule.toObject();
		m_schedules.emplace_back(scheduleObj);
	}
}

void PlayListManager::WritePlaylists(QJsonObject& json) const
{
	QJsonArray playlistArray;
	for (auto const& playlist : m_playlists)
	{
		QJsonObject playlistObj;
		playlist.write(playlistObj);
		playlistArray.append(playlistObj);
	}
	json["playlists"] = playlistArray;
}

void PlayListManager::WriteSchedules(QJsonObject& json) const
{
	QJsonArray scheduleArray;
	for (auto const& schedule : m_schedules)
	{
		QJsonObject scheduleObj;
		schedule.write(scheduleObj);
		scheduleArray.append(scheduleObj);
	}
	json["schedules"] = scheduleArray;
}

[[nodiscard]] std::optional< std::reference_wrapper< PlayList const > > PlayListManager::GetPlayList(int index) const
{
	if (index < 0 || index > m_playlists.size())
	{
		return std::nullopt;
	}
	return m_playlists.at(index);
}

[[nodiscard]] std::optional< std::reference_wrapper< PlayList const > > PlayListManager::GetPlayList(QString const& name) const
{
	if (auto const found{ std::find_if(m_playlists.cbegin(),m_playlists.cend(),
											[&name](auto& c) { return c.Name.compare(name, Qt::CaseInsensitive) == 0; }) };
		found != m_playlists.cend())
	{
		return *found;
	}
	return std::nullopt;
}

QStringList PlayListManager::GetPlayLists() const 
{
	QStringList playLists;
	std::transform(m_playlists.cbegin(), m_playlists.cend(), std::back_inserter(playLists),
		[](auto const& pl) { return pl.Name; });

	return playLists;
}

void PlayListManager::CheckSchedule()
{
	if (m_status != PlaybackStatus::Stopped)
	{
		return; 
	}
	auto const& current = QDateTime::currentDateTime();

	for (auto const& schedule : m_schedules)
	{
		if (current.date() < schedule.StartDate || current.date() > schedule.EndDate)
		{
			continue;
		}
		if (current.time() < schedule.StartTime || current.time() > schedule.EndTime)
		{
			continue;
		}
		if (!schedule.Days.contains(QDate::shortDayName(current.date().dayOfWeek()))) 
		{
			continue;
		}
		if (schedule.PlayListName == m_currentPlaylist)
		{
			PlayNextSequence();
			break;
		}
		PlayNewPlaylist(schedule.PlayListName);
		break;
	}
}

void PlayListManager::PlayNextSequence()
{
	if (auto const& playlistRef = GetPlayList(m_currentPlaylist); playlistRef)
	{
		auto const& playlist = playlistRef->get();
		if(playlist.PlayListItems.size() == 0)
		{
			m_logger->warn("Playlist is empty: {}", m_currentPlaylist.toStdString());
			return;
		}

		emit PlaySequenceSend(playlist.PlayListItems[m_nextSequenceIdx].SequenceFile,
			playlist.PlayListItems[m_nextSequenceIdx].MediaFile);
		++m_nextSequenceIdx;
		if (m_nextSequenceIdx >= playlist.PlayListItems.size())
		{
			m_nextSequenceIdx = 0;
		}
	}
}

void PlayListManager::PlayNewPlaylist(QString const& playlistName)
{
	if (auto const& playlistRef = GetPlayList(playlistName); playlistRef)
	{
		auto const& playlist = playlistRef->get();
		if(playlist.PlayListItems.size() == 0)
		{
			m_logger->warn("Playlist is empty: {}", playlistName.toStdString());
			return;
		}
		m_nextSequenceIdx = 0;
		m_currentPlaylist = playlistName;

		emit PlaySequenceSend(playlist.PlayListItems[m_nextSequenceIdx].SequenceFile,
			playlist.PlayListItems[m_nextSequenceIdx].MediaFile);
		++m_nextSequenceIdx;
		if (m_nextSequenceIdx >= playlist.PlayListItems.size())
		{
			m_nextSequenceIdx = 0;
		}
	}
}
