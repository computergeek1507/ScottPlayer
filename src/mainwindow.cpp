#include "mainwindow.h"

#include "./ui_mainwindow.h"

#include "config.h"

#include "addschedule.h"

#include "./players/PlayList.h"
#include "./players/PlayListItem.h"
#include "./players/Schedule.h"

#include <QMessageBox>
#include <QDesktopServices>
#include <QSettings>
#include <QFileDialog>
#include <QTextStream>
#include <QListWidget>
#include <QListWidgetItem>
#include <QTableWidget>
#include <QThread>
#include <QInputDialog>
#include <QCommandLineParser>
#include <QTimer>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QStandardPaths>
#include <QOperatingSystemVersion>

#include "spdlog/spdlog.h"

#include "spdlog/sinks/qt_sinks.h"
#include "spdlog/sinks/rotating_file_sink.h"

#include <filesystem>
#include <utility>

enum class PlaylistColumn: int { SeqFile=0, MediaFile };
enum class ScheduleColumn: int { Playlist=0, StartTime, EndTime, StartDate, EndDate, Days, Enabled };
enum class ControllerColumn: int { Type=0, Address, Channels, Enabled };

MainWindow::MainWindow(QWidget* parent)
	: QMainWindow(parent)
	, m_ui(new Ui::MainWindow)
{
	QCoreApplication::setApplicationName(PROJECT_NAME);
	QCoreApplication::setApplicationVersion(PROJECT_VER);
	m_ui->setupUi(this);

	auto const log_name{ "log.txt" };

	m_appdir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
	std::filesystem::create_directory(m_appdir.toStdString());
	QString logdir = m_appdir + "/log/";
	std::filesystem::create_directory(logdir.toStdString());

	try
	{
		auto file{ std::string(logdir.toStdString() + log_name) };
		auto rotating = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(file, 1024 * 1024, 5, false);

		m_logger = std::make_shared<spdlog::logger>("scottplayer", rotating);
		m_logger->flush_on(spdlog::level::debug);
		m_logger->set_level(spdlog::level::debug);
		m_logger->set_pattern("[%D %H:%M:%S] [%L] %v");
		spdlog::register_logger(m_logger);
	}
	catch (std::exception& /*ex*/)
	{
		QMessageBox::warning(this, "Logger Failed", "Logger Failed To Start.");
	}

	setWindowTitle(windowTitle() + " v" + PROJECT_VER);

	m_settings = std::make_unique< QSettings>(m_appdir + "/settings.ini", QSettings::IniFormat);

	m_player = std::make_unique<SequencePlayer>();
	connect(m_player.get(), &SequencePlayer::AddController, this, &MainWindow::AddController_Received);
	connect(m_player.get(), &SequencePlayer::UpdateStatus, this, &MainWindow::UpdateStatus);
	connect(m_player.get(), &SequencePlayer::UpdateTime, this, &MainWindow::UpdatePlayback);

	m_playlists = std::make_unique<PlayListManager>();
	connect(m_player.get(), &SequencePlayer::UpdatePlaybackStatus, m_playlists.get(), &PlayListManager::UpdateStatus);
	connect(m_playlists.get(), &PlayListManager::PlaySequenceSend, m_player.get(), &SequencePlayer::LoadSequence);
	connect(m_playlists.get(), &PlayListManager::AddPlaylistSend, this, &MainWindow::AddPlaylist);
	connect(m_playlists.get(), &PlayListManager::DisplayPlaylistSend, this, &MainWindow::RedrawPlaylist);
	connect(m_playlists.get(), &PlayListManager::MessageSend, this, &MainWindow::UpdateStatus);
	connect(m_playlists.get(), &PlayListManager::SelectSequenceSend, this, &MainWindow::SelectSequence);
	connect(m_playlists.get(), &PlayListManager::DisplayScheduleSend, this, &MainWindow::RedrawSchedule);

	auto lastfolder{ m_settings->value("last_folder").toString() };

	if (QDir(lastfolder).exists())
	{
		m_player->LoadConfigs(lastfolder);
		m_playlists->LoadPlayLists(lastfolder);
		m_showfolder = lastfolder;
		RedrawSchedule();
	}

	bool enabled = m_settings->value("multisync", "true").toBool();
	m_player->SetMultisync(enabled);
	m_ui->actionMultisync->setChecked(enabled);
}

MainWindow::~MainWindow()
{
	delete m_ui;
}

void MainWindow::on_actionSet_Show_Folder_triggered()
{
	QFileDialog dialog;
	dialog.setFileMode(QFileDialog::Directory);
	dialog.setOption(QFileDialog::ShowDirsOnly);
	auto lastfolder{ m_settings->value("last_folder",QStandardPaths::writableLocation(QStandardPaths::DesktopLocation)).toString() };

	QString const folder = dialog.getExistingDirectory(this, "Select xLight Show Folder", lastfolder, QFileDialog::ShowDirsOnly);
	if (!folder.isEmpty() && QDir(folder).exists())
	{
		ClearListData();
		m_showfolder = folder;
		m_player->LoadConfigs(folder);
		m_settings->setValue("last_folder", folder);
		m_settings->sync();
	}
}

void MainWindow::on_actionSave_triggered()
{
	if(m_showfolder.isEmpty())
	{
		m_logger->error("Show Folder is not set");
		return;
	}
	m_playlists->SavePlayLists(m_showfolder);
}

void MainWindow::on_actionStop_Sequence_triggered()
{
	m_player->StopSequence();
}

void MainWindow::on_actionClose_triggered()
{
	close();
}

void MainWindow::on_actionMultisync_triggered()
{
	bool enabled = m_ui->actionMultisync->isChecked();
	m_player->SetMultisync(enabled);
	m_settings->setValue("multisync", enabled);
	m_settings->sync();
}

void MainWindow::on_actionAbout_triggered()
{
	QString text = QString("Scott Player v%1<br>QT v%2<br><br>Icons by:")
		.arg(PROJECT_VER, QT_VERSION_STR) +
		QStringLiteral("<br><a href='http://www.famfamfam.com/lab/icons/silk/'>www.famfamfam.com</a>");
	//http://www.famfamfam.com/lab/icons/silk/
	QMessageBox::about(this, "About Scott Player", text);
}

void MainWindow::on_actionOpen_Logs_triggered()
{
	QDesktopServices::openUrl(QUrl::fromLocalFile(m_appdir + "/log/"));
}

void MainWindow::on_pb_addPlaylist_clicked()
{
	bool ok;
	QString text = QInputDialog::getText(this, "PlayList Name",
		"PlayList Name", QLineEdit::Normal,
		"Main", &ok);
	if (ok && !text.isEmpty())
	{
		m_playlists->AddPlaylistName(text);
	}
}

void MainWindow::on_pb_deletePlaylist_clicked()
{
	m_playlists->DeletePlayList(m_ui->cb_playlists->currentData().toInt());
	m_ui->cb_playlists->removeItem(m_ui->cb_playlists->currentIndex());
}

void MainWindow::on_pb_addSequence_clicked()
{
	QString const fseqFile = QFileDialog::getOpenFileName(this, "Select FSEQ File", m_settings->value("last_fseq").toString(), tr("FSEQ Files (*.fseq);;All Files (*.*)"));
	if (!fseqFile.isEmpty())
	{
		QString audio;
		FSEQFile* seqFile = FSEQFile::openFSEQFile(fseqFile.toStdString());
		if (seqFile != nullptr)
		{
			audio = QString::fromStdString(seqFile->getMediaFilename());
		}
		m_playlists->AddSequence(fseqFile, audio, m_ui->cb_playlists->currentData().toInt());
		m_settings->setValue("last_fseq", fseqFile);
		m_settings->sync();
		delete seqFile;
	}
}

void MainWindow::on_pb_removeSequence_clicked()
{
	m_playlists->DeleteSequence(m_ui->cb_playlists->currentData().toInt(), m_ui->tw_playlists->currentRow());
}

void MainWindow::on_pb_moveUp_clicked()
{
	m_playlists->MoveSequenceUp(m_ui->cb_playlists->currentData().toInt(), m_ui->tw_playlists->currentRow());
}

void MainWindow::on_pb_moveDown_clicked()
{
	m_playlists->MoveSequenceDown(m_ui->cb_playlists->currentData().toInt(), m_ui->tw_playlists->currentRow());
}

void MainWindow::on_pb_playSequence_clicked()
{
	m_playlists->PlaySequence(m_ui->cb_playlists->currentData().toInt(),m_ui->tw_playlists->currentRow());
}

void MainWindow::on_pb_stopSequence_clicked()
{
	m_player->StopSequence();
}

void MainWindow::on_pb_addSchedule_clicked()
{
	AddSchedule pn(this);
	if (pn.Load(m_playlists->GetPlayLists()))
	{
		m_playlists->AddSchedule(pn.GetSchedule());
	}
}

void MainWindow::on_pb_editSchedule_clicked()
{
	if(-1 == m_ui->tw_schedules->currentRow())
	{
		return;
	}
	auto row = m_ui->tw_schedules->currentRow();
	AddSchedule pn(this);
	if (pn.LoadData(m_playlists->GetPlayLists(), m_playlists->GetSchedules().at(row)))
	{
		m_playlists->EditSchedule(row, pn.GetSchedule());
	}
}

void MainWindow::on_pb_deleteSchedule_clicked()
{
	m_playlists->DeleteSchedule(m_ui->tw_schedules->currentRow());
}

void MainWindow::on_pb_sch_moveUp_clicked()
{
	m_playlists->MoveScheduleUp( m_ui->tw_schedules->currentRow());
}

void MainWindow::on_pb_sch_moveDown_clicked()
{
	m_playlists->MoveScheduleDown( m_ui->tw_schedules->currentRow());
}

void MainWindow::on_tw_playlists_cellDoubleClicked(int row, int column) 
{
	m_playlists->PlaySequence(m_ui->cb_playlists->currentData().toInt(), row);
}

void MainWindow::on_tw_schedules_cellDoubleClicked(int row, int column)
{
	AddSchedule pn(this);
	if (pn.LoadData(m_playlists->GetPlayLists(), m_playlists->GetSchedules().at(row)))
	{
		m_playlists->EditSchedule(row, pn.GetSchedule());
	}
}

void MainWindow::on_tw_controllers_cellDoubleClicked(int row, int column)
{
	auto ip = m_ui->tw_controllers->item(row, std::to_underlying(ControllerColumn::Address))->text();

	QDesktopServices::openUrl(QUrl("http://" + ip));
}

void MainWindow::AddController_Received(bool enabled, QString const& type, QString const& ip, QString const& channel)
{
	auto SetItem = [&](int row, ControllerColumn col, QString const& text)
	{
		m_ui->tw_controllers->setItem(row, std::to_underlying(col), new QTableWidgetItem());
		m_ui->tw_controllers->item(row, std::to_underlying(col))->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
		m_ui->tw_controllers->item(row, std::to_underlying(col))->setText(text);
	};

	int row = m_ui->tw_controllers->rowCount();
	m_ui->tw_controllers->insertRow(row);
	SetItem(row, ControllerColumn::Enabled, enabled ? "true" : "false");
	SetItem(row, ControllerColumn::Type, type);
	SetItem(row, ControllerColumn::Address, ip);
	SetItem(row, ControllerColumn::Channels, channel);
	m_ui->tw_controllers->resizeColumnsToContents();
}

void MainWindow::SelectSequence(int index) 
{
	m_ui->tw_playlists->selectRow(index);
}

void MainWindow::on_cb_playlists_currentIndexChanged( int index )
{
	RedrawPlaylist(index);
}

void MainWindow::RedrawPlaylist(int index)
{
	m_ui->tw_playlists->clearContents();
	auto SetItem = [&](int row, PlaylistColumn col, QString const& text)
	{
		m_ui->tw_playlists->setItem(row, std::to_underlying(col), new QTableWidgetItem());
		m_ui->tw_playlists->item(row, std::to_underlying(col))->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
		m_ui->tw_playlists->item(row, std::to_underlying(col))->setText(text);
		m_ui->tw_playlists->item(row, std::to_underlying(col))->setData(Qt::UserRole, row);
	};
	if(auto const& playlistRef = m_playlists->GetPlayList(index); playlistRef)
	{
		auto const& playlist = playlistRef->get();

		m_ui->tw_playlists->setRowCount(static_cast<int>(playlist.PlayListItems.size()));
		int row{ 0 };
		for(auto const& item : playlist.PlayListItems)
		{
			
			SetItem(row, PlaylistColumn::SeqFile, GetFileName(item.SequenceFile));
			SetItem(row, PlaylistColumn::MediaFile, GetFileName(item.MediaFile));
			++row;
		}
	}
	m_ui->tw_playlists->resizeColumnsToContents();

}

void MainWindow::RedrawSchedule()
{
	m_ui->tw_schedules->clearContents();
	auto SetItem = [&](int row, ScheduleColumn col, QString const& text)
	{
		m_ui->tw_schedules->setItem(row, std::to_underlying(col), new QTableWidgetItem());
		m_ui->tw_schedules->item(row, std::to_underlying(col))->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
		m_ui->tw_schedules->item(row, std::to_underlying(col))->setText(text);
		m_ui->tw_schedules->item(row, std::to_underlying(col))->setData(Qt::UserRole, row);
	};
	
		m_ui->tw_schedules->setRowCount(static_cast<int>(m_playlists->GetSchedules().size()));
		int row{ 0 };
		for (auto const& shed : m_playlists->GetSchedules())
		{
			SetItem(row, ScheduleColumn::Playlist, shed.PlayListName);
			SetItem(row, ScheduleColumn::StartTime, shed.StartTime.toString());
			SetItem(row, ScheduleColumn::EndTime, shed.EndTime.toString());
			SetItem(row, ScheduleColumn::StartDate, shed.StartDate.toString());
			SetItem(row, ScheduleColumn::EndDate, shed.EndDate.toString());
			SetItem(row, ScheduleColumn::Days, shed.Days.join(","));
			SetItem(row, ScheduleColumn::Enabled, shed.Enabled ? "true" : "false");
			++row;
		}
	
	m_ui->tw_schedules->resizeColumnsToContents();
}

void MainWindow::ClearListData()
{
	while (m_ui->tw_controllers->rowCount() != 0)
	{
		m_ui->tw_controllers->removeRow(0);
	}
}

void MainWindow::AddPlaylist(QString const& Playlist, int index)
{
	m_ui->cb_playlists->addItem(Playlist, index);
}

void MainWindow::UpdateStatus(QString const& message)
{
	m_ui->lb_Status->setText(message);
	m_logger->debug(message.toStdString());
}

void MainWindow::UpdatePlayback(QString const& sequenceName, int elapsedMS, int durationMS)
{
	m_ui->lb_Status->setText(QString("Playing %1 %2/%3").arg(sequenceName).arg(FormatTime(elapsedMS)).arg(FormatTime(durationMS)));
}

void MainWindow::LogMessage(QString const& message, spdlog::level::level_enum llvl)
{
	m_logger->log(llvl, message.toStdString());
}

QString MainWindow::FormatTime(int ticksMS) const
{
	return QTime::fromMSecsSinceStartOfDay( ticksMS ).toString("mm:ss");
}

QString MainWindow::GetFileName(QString const& path) const
{
	if(path.isEmpty()) return QString();
	return QFileInfo( path ).fileName();
}
