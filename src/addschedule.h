#ifndef ADD_SCHEDULE_H
#define ADD_SCHEDULE_H

#include "qxtcheckcombobox.h"

#include <QDialog>

#include "ui_addschedule.h"

class AddSchedule : public QDialog
{
	Q_OBJECT

public:
	explicit AddSchedule(QWidget* parent = nullptr);
	~AddSchedule();

	int Load(QStringList const& playlists);

	void SetData(QString const& playlist, QTime const& startTime, QTime const& endTime, QDate const& startDate, QDate const& endDate, QStringList const& days);

	[[nodiscard]] QString GetPlayList() const { return ui->cb_playlists->currentText(); }
	[[nodiscard]] QTime GetStartTime() const { return ui->te_startTime->time(); }
	[[nodiscard]] QTime GetEndTime() const { return ui->te_endTime->time(); }
	[[nodiscard]] QDate GetStartDate() const { return ui->de_startDate->date(); }
	[[nodiscard]] QDate GetEndDate() const { return ui->de_endDate->date(); }
	[[nodiscard]] QStringList GetDays() const { return m_days; }

public Q_SLOTS:
	void on_buttonBox_accepted();
	void ChangeDays(QStringList const& items);

private:
	Ui::AddScheduleDialog* ui;
	QxtCheckComboBox* m_daysCheckBox;
	QStringList m_days;
};

#endif // ADD_SCHEDULE_H