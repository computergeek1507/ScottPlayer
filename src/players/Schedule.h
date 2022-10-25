#ifndef SCHEDULE_H
#define SCHEDULE_H

#include <QString>
#include <QDateTime>
#include <QJsonObject>
#include <QJsonValue>

struct Schedule
{
	//QString Name;
	QString PlayListName;
	QTime StartTime;
	QTime EndTime;
	QDate StartDate;
	QDate EndDate;
	QStringList Days;

	Schedule() = default;

	Schedule(QString const& playlist, QTime const& startTime, QTime const& endTime, QDate const& startDate, QDate const& endDate, QStringList const& days ) :
		PlayListName(playlist), StartTime(startTime), EndTime(endTime), StartDate(startDate), EndDate(endDate), Days(days)
	{

	}

	Schedule(QJsonObject const& json)
	{
		read(json);
	}

	void write(QJsonObject& json) const
	{
		//json["name"] = Name;
		json["playList"] = PlayListName;
		json["startTime"] = QJsonValue::fromVariant(StartTime);
		json["endTime"] = QJsonValue::fromVariant(EndTime);
		json["startDate"] = QJsonValue::fromVariant(StartDate);
		json["endDate"] = QJsonValue::fromVariant(EndDate);
		json["days"] = QJsonValue::fromVariant(Days);
	}

	void read(const QJsonObject& json)
	{
		//Name = json["name"].toString();
		PlayListName = json["playList"].toString();
		StartTime = json["startTime"].toVariant().toTime();
		EndTime = json["endTime"].toVariant().toTime();
		StartDate = json["startDate"].toVariant().toDate();
		EndDate = json["endDate"].toVariant().toDate();
		Days = json["days"].toVariant().toStringList();
	}
};

#endif