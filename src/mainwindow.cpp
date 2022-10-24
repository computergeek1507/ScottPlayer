#include "mainwindow.h"

#include "./ui_mainwindow.h"

#include "config.h"

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
#include <QJsonObject>
#include <QJsonDocument>
#include <QJsonArray>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QStandardPaths>
#include <QOperatingSystemVersion>

#include "spdlog/spdlog.h"

#include "spdlog/sinks/qt_sinks.h"
#include "spdlog/sinks/rotating_file_sink.h"

#include <filesystem>

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
	connect(m_player.get(), &SequencePlayer::AddController, this, &MainWindow::on_AddController);
	connect(m_player.get(), &SequencePlayer::UpdateStatus, this, &MainWindow::UpdateStatus);
	connect(m_player.get(), &SequencePlayer::UpdateTime, this, &MainWindow::UpdatePlayback);

	auto lastfolder{ m_settings->value("last_folder").toString() };

	if (QDir(lastfolder).exists())
	{
		m_player->LoadConfigs(lastfolder);
		m_showfolder = lastfolder;
	}
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

//mostly for testing
void MainWindow::on_actionPlay_Sequence_triggered()
{
	QString const fseqFile = QFileDialog::getOpenFileName(this, "Select FSEQ File", m_settings->value("last_fseq").toString(), tr("FSEQ Files (*.fseq);;All Files (*.*)"));
	if (!fseqFile.isEmpty())
	{
		m_player->LoadSequence(fseqFile);
		m_settings->setValue("last_fseq", fseqFile);
		m_settings->sync();
	}
}

void MainWindow::on_actionStop_Sequence_triggered()
{
	m_player->StopSequence();
}

void MainWindow::on_actionClose_triggered()
{
	close();
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

void MainWindow::on_AddController(QString const& type, QString const& ip, QString const& channel)
{
	auto SetItem = [&](int row, int col, QString const& text)
	{
		m_ui->twControllers->setItem(row, col, new QTableWidgetItem());
		m_ui->twControllers->item(row, col)->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
		m_ui->twControllers->item(row, col)->setText(text);
	};

	int row = m_ui->twControllers->rowCount();
	m_ui->twControllers->insertRow(row);
	SetItem(row, 0, type);
	SetItem(row, 1, ip);
	SetItem(row, 2, channel);
	m_ui->twControllers->resizeColumnsToContents();
}

void MainWindow::ClearListData()
{
	while (m_ui->twControllers->rowCount() != 0)
	{
		m_ui->twControllers->removeRow(0);
	}
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