#include "PlayListManager.h"

#include "PlayList.h"
#include "Schedule.h"

#include <QFile>
#include <QDir>
#include <QTextStream>
#include <QJsonObject>
#include <QJsonDocument>
#include <QJsonArray>

PlayListManager::PlayListManager():
		m_logger(spdlog::get("scottplayer"))
{

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
