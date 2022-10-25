#include "addschedule.h"

#include <QDate>

AddSchedule::AddSchedule(QWidget* parent) :
	ui(new Ui::AddScheduleDialog),
	m_daysCheckBox(new QxtCheckComboBox())
{
	ui->setupUi(this);
	
	for (int i = 0; i < Qt::DayOfWeek::Sunday; ++i)
	{
		QString weekDay = QDate::shortDayName(i + 1);
		m_daysCheckBox->addItem(weekDay);
		m_daysCheckBox->setItemCheckState(i, Qt::Checked);
		m_days.append(weekDay);
	}

	m_daysCheckBox->setSizeAdjustPolicy(QComboBox::AdjustToContentsOnFirstShow);
	m_daysCheckBox->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Fixed);
	((QHBoxLayout*)ui->horizontalLayout)->addWidget(m_daysCheckBox, 0, 0);

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

void AddSchedule::SetData(QString const& playlist, QTime const& startTime, QTime const& endTime, QDate const& startDate, QDate const& endDate, QStringList const& days)
{

}

void AddSchedule::ChangeDays(QStringList const& items)
{
	m_days = items;
}

void AddSchedule::on_buttonBox_accepted()
{
	
	this->accept();
}