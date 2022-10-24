#ifndef PLAYLISTITEM_H
#define PLAYLISTITEM_H

#include <QString>
#include <QJsonObject>

struct PlayListItem
{
	QString SequenceFile;
	QString MediaFile;

	PlayListItem() = default;

	PlayListItem(QString const& seq, QString const& media):
		SequenceFile(seq), MediaFile(media)
	{
	}

	explicit PlayListItem(QJsonObject const& json)
	{
		read(json);
	}

	void write(QJsonObject& json) const
	{
		json["seq"] = SequenceFile;
		json["media"] = MediaFile;
	}

	void read(const QJsonObject& json)
	{
		SequenceFile = json["seq"].toString();
		MediaFile = json["media"].toString();
	}
};

#endif