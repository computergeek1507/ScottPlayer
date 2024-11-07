#include "addschedule.h"

#include "./players/Schedule.h"

#include <QDate>

#if (QT_VERSION >= QT_VERSION_CHECK(6,0,0))
#include <QLocale>
#endif

AddSchedule::AddSchedule(QWidget* parent):
	ui(new Ui::AddScheduleDialog),
	m_daysCheckBox(new QxtCheckComboBox())
{
	ui->setupUi(this);
	
	for (int i = 0; i < Qt::DayOfWeek::Sunday; ++i)
	{
#if (QT_VERSION >= QT_VERSION_CHECK(6,0,0))
		QString weekDay = QLocale().dayName(i + 1, QLocale::ShortFormat);
#else
		QString weekDay = QDate::shortDayName(i + 1);
#endif
		m_daysCheckBox->addItem(weekDay);
		m_daysCheckBox->setItemCheckState(i, Qt::Checked);
		m_days.append(weekDay);
	}

	m_daysCheckBox->setSizeAdjustPolicy(QComboBox::AdjustToContentsOnFirstShow);
	m_daysCheckBox->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Fixed);
	((QHBoxLayout*)ui->horizontalLayout)->addWidget(m_daysCheckBox, 0);

	connect(m_daysCheckBox, &QxtCheckComboBox::checkedItemsChanged, this, &AddSchedule::ChangeDays);
}

AddSchedule::~AddSchedule()
{
	delete ui;
}

int AddSchedule::Load(QStringList const& playlists)
{
	ui->cb_playlists->clear();
	ui->cb_playlists->addItems(playlists);
	return this->exec();
}

int AddSchedule::LoadData(QStringList const& playlists, Schedule const& schedule)
{
	ui->cb_playlists->clear();
	ui->cb_playlists->addItems(playlists);

	if(-1 != ui->cb_playlists->findText(schedule.PlayListName))
	{
		ui->cb_playlists->setCurrentIndex(ui->cb_playlists->findText(schedule.PlayListName));
	}
	m_daysCheckBox->clearCheckedItems();
	m_daysCheckBox->setCheckedItems(schedule.Days);

	ui->te_startTime->setTime(schedule.StartTime);
	ui->te_endTime->setTime(schedule.EndTime);
	ui->de_startDate->setDate(schedule.StartDate);
	ui->de_endDate->setDate(schedule.EndDate);
	ui->cb_enabled->setChecked(schedule.Enabled);
	return this->exec();
}

Schedule AddSchedule::GetSchedule() const
{
	return Schedule(ui->cb_playlists->currentText(), 
		ui->te_startTime->time(),ui->te_endTime->time(),
		ui->de_startDate->date(),ui->de_endDate->date(),
		m_days,ui->cb_enabled->isChecked());
}

void AddSchedule::ChangeDays(QStringList const& items)
{
	m_days = items;
}

void AddSchedule::on_buttonBox_accepted()
{
	if(ui->de_startDate->date() > ui->de_endDate->date())
	{
		return;
	}
	if(m_days.isEmpty())
	{
		return;
	}
	if(ui->cb_playlists->currentText().isEmpty())
	{
		return;
	}
	this->accept();
}