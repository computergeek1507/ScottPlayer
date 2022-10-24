#ifndef SCHEDULE_H
#define SCHEDULE_H

#include <QString>
#include <QDateTime>
#include <QJsonObject>
#include <QJsonValue>

struct Schedule
{
	QString Name;
	QString PlayListName;
	QDateTime StartTime;
	QDateTime EndTime;

	Schedule() = default;

	Schedule(QJsonObject const& json)
	{
		read(json);
	}

	void write(QJsonObject& json) const
	{
		json["name"] = Name;
		json["playList"] = PlayListName;
		json["start"] = QJsonValue::fromVariant(StartTime);
		json["end"] = QJsonValue::fromVariant(EndTime);
	}

	void read(const QJsonObject& json)
	{
		Name = json["name"].toString();
		PlayListName = json["playList"].toString();
		StartTime = json["start"].toVariant().toDateTime();
		EndTime = json["end"].toVariant().toDateTime();
	}
};

#endif