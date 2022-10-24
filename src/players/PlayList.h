#ifndef PLAYLIST_H
#define PLAYLIST_H

#include <QString>
#include <vector>

#include "PlayListItem.h"

#include <QJsonObject>
#include <QJsonArray>

struct PlayList
{
	PlayList() = default;

	PlayList(QString const& name):Name(name)
	{ }

	PlayList(QJsonObject const& json)
	{
		read(json);
	}

	std::vector<PlayListItem> PlayListItems;
	QString Name;

	void write(QJsonObject& json) const
	{
		json["name"] = Name;
		QJsonArray itemArray;
		for (auto const& item : PlayListItems)
		{
			QJsonObject itemObj;
			item.write(itemObj);
			itemArray.append(itemObj);
		}
		json["items"] = itemArray;
	}

	void read(const QJsonObject& json)
	{
		Name = json["name"].toString();
		QJsonArray itemArray = json["items"].toArray();
		for (auto const& item : itemArray)
		{
			QJsonObject itemObj = item.toObject();
			PlayListItems.emplace_back(itemObj);
		}
	}
};

#endif