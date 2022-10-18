#include "mainwindow.h"

#include "./ui_mainwindow.h"

#include "cape_utils.h"

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

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
	QCoreApplication::setApplicationName(PROJECT_NAME);
    QCoreApplication::setApplicationVersion(PROJECT_VER);
    ui->setupUi(this);

	auto const log_name{ "log.txt" };

	appdir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
	std::filesystem::create_directory(appdir.toStdString());
	QString logdir = appdir + "/log/";
	std::filesystem::create_directory(logdir.toStdString());

	try
	{
		auto file{ std::string(logdir.toStdString() + log_name) };
		auto rotating = std::make_shared<spdlog::sinks::rotating_file_sink_mt>( file, 1024 * 1024, 5, false);

		logger = std::make_shared<spdlog::logger>("capeeepromviewer", rotating);
		logger->flush_on(spdlog::level::debug);
		logger->set_level(spdlog::level::debug);
		logger->set_pattern("[%D %H:%M:%S] [%L] %v");
		spdlog::register_logger(logger);
	}
	catch (std::exception& /*ex*/)
	{
		QMessageBox::warning(this, "Logger Failed", "Logger Failed To Start.");
	}

	setWindowTitle(windowTitle() + " v" + PROJECT_VER);

	settings = std::make_unique< QSettings>(appdir + "/settings.ini", QSettings::IniFormat);

	RedrawRecentList();
	connect(ui->comboBoxCape, &QComboBox::currentTextChanged, this, &MainWindow::RedrawStringPortList);

	if (QOperatingSystemVersion::current().type() == QOperatingSystemVersion::OSType::Windows)
	{
		int micro = QOperatingSystemVersion::current().microVersion();

		if (micro < 17134)//Windows 10 1803 April 2018 Update
		{
			QMessageBox::warning(this, "Windows Version Error", "Your Verison of Windows is too old.\nPlease Update to Windows 10 1803 April 2018 Update or Newer");
		}
	}
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_actionOpen_EEPROM_triggered()
{
	QString const EEPROM = QFileDialog::getOpenFileName(this, "Select EEPROM File", settings->value("last_project").toString(), tr("EEPROM Files (*.bin *.eeprom);;All Files (*.*)"));
	if (!EEPROM.isEmpty())
	{
		LoadEEPROM(EEPROM);
	}
}

void MainWindow::on_actionDownload_EEPROM_triggered()
{
	bool ssl = QSslSocket::supportsSsl();
	QString const sslFile = QSslSocket::sslLibraryBuildVersionString();

	if (!ssl)
	{
		QString const text = QStringLiteral("OpenSSL was not found on your computer, This Feature will not work without it.<br>Please Install " ) + sslFile + QStringLiteral("<br><a href='http://slproweb.com/products/Win32OpenSSL.html'>OpenSSL Download</a>");
		QMessageBox::warning(this, "OpenSSL", text);
		return;
	}
	auto firmwares = GetFirmwareURLList();
	bool ok;
	QString firmware = QInputDialog::getItem(this, "Select FPP Firmware", "Select FPP Firmware", firmwares.keys(), 0, false, &ok);

	if (ok && !firmware.isEmpty())
	{
		DownloadFirmware(firmware, firmwares.value(firmware));
	}
}

void MainWindow::on_actionClose_triggered()
{
	close();
}

void MainWindow::on_actionAbout_triggered()
{
	QString text = QString("Cape EEPROM Viewer v%1<br>QT v%2<br><br>Icons by:")
		.arg(PROJECT_VER, QT_VERSION_STR) +
		QStringLiteral("<br><a href='http://www.famfamfam.com/lab/icons/silk/'>www.famfamfam.com</a>");
		//http://www.famfamfam.com/lab/icons/silk/
	QMessageBox::about( this, "About Cape EEPROM Viewer", text );
}

void MainWindow::on_actionOpen_Logs_triggered()
{
	QDesktopServices::openUrl(QUrl::fromLocalFile(appdir + "/log/"));
}

void MainWindow::on_menuRecent_triggered()
{
	auto recentItem = qobject_cast<QAction*>(sender());
	if (recentItem && !recentItem->data().isNull())
	{
		auto const project = qvariant_cast<QString>(recentItem->data());
		LoadEEPROM(project);
	}
}

void MainWindow::on_actionClear_triggered()
{
	ui->menuRecent->clear();
	settings->remove("Recent_ProjectsList");

	ui->menuRecent->addSeparator();
	ui->menuRecent->addAction(ui->actionClear);
}

void MainWindow::LoadEEPROM(QString const& filepath)
{
	settings->setValue("last_project", filepath);
	settings->sync();

	QFileInfo proj(filepath);
	m_cape = cape_utils::parseEEPROM(filepath.toStdString());

	ui->leProject->setText(m_cape.AsString().c_str());
	
	AddRecentList(proj.absoluteFilePath());

	ReadCapeInfo(m_cape.folder.c_str());
	CreateStringsList(m_cape.folder.c_str());
	ReadGPIOFile(m_cape.folder.c_str());
	ReadOtherFile(m_cape.folder.c_str());
}

void MainWindow::ReadCapeInfo(QString const& folder)
{
	ui->textEditCapeInfo->clear();
	QFile infoFile(folder + "/cape-info.json");
	if (!infoFile.exists())
	{
		LogMessage("cape-info file not found", spdlog::level::level_enum::err);
		return;
	}

	if (!infoFile.open(QIODevice::ReadOnly))
	{
		LogMessage("Error Opening: cape-info.json" , spdlog::level::level_enum::err);
		return;
	}

	QByteArray saveData = infoFile.readAll();
	ui->textEditCapeInfo->setText(saveData);
}

void MainWindow::CreateStringsList(QString const& folder)
{
	QDir directory(folder + "/strings");
	auto const& stringFiles = directory.entryInfoList(QStringList() << "*.json" , QDir::Files);

	ui->comboBoxCape->clear();
	
	for (auto const& file : stringFiles)
	{
		ui->comboBoxCape->addItem(file.fileName());
	}
}

void MainWindow::ReadGPIOFile(QString const& folder) 
{
	ui->twGPIO->clearContents();
	ui->twGPIO->setRowCount(0);
	//C:\Users\scoot\Desktop\BBB16-220513130003-eeprom\tmp\defaults\config\gpio.json
	auto SetItem = [&](int row, int col, QString const& text)
	{
		ui->twGPIO->setItem(row, col, new QTableWidgetItem());
		ui->twGPIO->item(row, col)->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
		ui->twGPIO->item(row, col)->setText(text);
	};

	QFile jsonFile(folder + "/defaults/config/gpio.json");
	if (!jsonFile.exists())
	{
		LogMessage("file not found gpio.json", spdlog::level::level_enum::err);
		return;
	}
	if (!jsonFile.open(QIODevice::ReadOnly))
	{
		LogMessage("Error Opening: gpio.json", spdlog::level::level_enum::err);
		return;
	}

	QByteArray saveData = jsonFile.readAll();

	QJsonDocument loadDoc(QJsonDocument::fromJson(saveData));
	QJsonArray mappingArray = loadDoc.array();

	ui->twGPIO->setRowCount(static_cast<int>(mappingArray.size()));
	int row{ 0 };

	for (auto const& mapp : mappingArray)
	{
		QJsonObject mapObj = mapp.toObject();
		QString pin = mapObj["pin"].toString();
		QString mode = mapObj["mode"].toString();
		SetItem(row, 0, pin);
		SetItem(row, 1, mode);
		if (mapObj.contains("desc"))
		{
			SetItem(row, 2, mapObj["desc"].toString());
		}

		if (mapObj.contains("rising"))
		{
			SetItem(row, 3, "rising");
			if (mapObj["rising"].toObject().contains("command")) 
			{
				SetItem(row, 4, mapObj["rising"].toObject()["command"].toString());
			}
			if (mapObj["rising"].toObject().contains("args") &&
				mapObj["rising"].toObject()["args"].toArray().size()>0)
			{
				SetItem(row, 5, mapObj["rising"].toObject()["args"].toArray()[0].toString());
			}
		}
		else if (mapObj.contains("falling"))
		{
			SetItem(row, 3, "falling");
			if (mapObj["falling"].toObject().contains("command"))
			{
				SetItem(row, 4, mapObj["falling"].toObject()["command"].toString());
			}
			if (mapObj["falling"].toObject().contains("args") &&
				mapObj["falling"].toObject()["args"].toArray().size() > 0)
			{
				SetItem(row, 5, mapObj["falling"].toObject()["args"].toArray()[0].toString());
			}
		}
		++row;
	}
}

void MainWindow::ReadOtherFile(QString const& folder)
{
	ui->twOther->clearContents();
	ui->twOther->setRowCount(0);
	//C:\Users\scoot\Desktop\BBB16-220513130003-eeprom\tmp\defaults\config\co-other.json
	auto SetItem = [&](int row, int col, QString const& text)
	{
		ui->twOther->setItem(row, col, new QTableWidgetItem());
		ui->twOther->item(row, col)->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
		ui->twOther->item(row, col)->setText(text);
	};

	QFile jsonFile(folder + "/defaults/config/co-other.json");
	if (!jsonFile.exists())
	{
		LogMessage("file not found co-other.json", spdlog::level::level_enum::err);
		return;
	}
	if (!jsonFile.open(QIODevice::ReadOnly))
	{
		LogMessage("Error Opening: co-other.json", spdlog::level::level_enum::err);
		return;
	}

	QByteArray saveData = jsonFile.readAll();
	QJsonDocument loadDoc(QJsonDocument::fromJson(saveData));
	QJsonArray mappingArray = loadDoc.object()["channelOutputs"].toArray();

	ui->twOther->setRowCount(static_cast<int>(mappingArray.size()));
	int row{ 0 };

	for (auto const& mapp : mappingArray)
	{
		QJsonObject mapObj = mapp.toObject();
		if (mapObj.contains("type"))
		{
			SetItem(row, 0, mapObj["type"].toString());
		}
		if (mapObj.contains("device")) 
		{
			SetItem(row, 1, mapObj["device"].toString());
		}
		++row;
	}
}

void MainWindow::RedrawStringPortList(QString const& strings)
{
	ui->twParts->clearContents();
	ui->twParts->setRowCount(0);
	auto SetItem = [&](int row, int col, QString const& text)
	{
		ui->twParts->setItem(row, col, new QTableWidgetItem());
		ui->twParts->item(row, col)->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
		ui->twParts->item(row, col)->setText(text);
	};

	if (strings.isEmpty() || m_cape.folder.empty())
	{
		return;
	}

	QFile jsonFile(QString(m_cape.folder.c_str()) + "/strings/" + strings);
	if (!jsonFile.exists())
	{
		LogMessage("file not found" + strings, spdlog::level::level_enum::err);
		return;
	}

	if (!jsonFile.open(QIODevice::ReadOnly))
	{
		LogMessage("Error Opening: " + strings, spdlog::level::level_enum::err);
		return;
	}

	QByteArray saveData = jsonFile.readAll();
	QJsonDocument loadDoc(QJsonDocument::fromJson(saveData));
	int tableSize{0};

	if (loadDoc.object().contains("outputs"))
	{
		tableSize += loadDoc.object()["outputs"].toArray().size();
	}

	if (loadDoc.object().contains("serial"))
	{
		tableSize += loadDoc.object()["serial"].toArray().size();
	}
	ui->twParts->setRowCount(tableSize);
	int row{ 0 };

	if (loadDoc.object().contains("outputs"))
	{
		QJsonArray mappingArray = loadDoc.object()["outputs"].toArray();
		for (auto const& mapp : mappingArray)
		{
			QJsonObject mapObj = mapp.toObject();
			SetItem(row, 0, "String " + QString::number(row + 1));
			if (mapObj.contains("pin"))
			{
				SetItem(row, 1, mapObj["pin"].toString());
			}
			++row;
		}
	}

	if (loadDoc.object().contains("serial"))
	{
		int serNum{ 1 };
		QJsonArray mappingArray = loadDoc.object()["serial"].toArray();
		for (auto const& mapp : mappingArray)
		{
			QJsonObject mapObj = mapp.toObject();
			SetItem(row, 0, "Serial " + QString::number(serNum));
			if (mapObj.contains("pin"))
			{
				SetItem(row, 1, mapObj["pin"].toString());
			}
			++row;
			++serNum;
		}
	}
}

void MainWindow::AddRecentList(QString const& file)
{
	auto recentProjectList = settings->value("Recent_ProjectsList").toStringList();

	recentProjectList.push_front(file);
	recentProjectList.removeDuplicates();
	if (recentProjectList.size() > 10)
	{
		recentProjectList.pop_back();
	}
	settings->setValue("Recent_ProjectsList", recentProjectList);
	settings->sync();
	RedrawRecentList();
}

void MainWindow::RedrawRecentList()
{
	ui->menuRecent->clear();
	auto recentProjectList = settings->value("Recent_ProjectsList").toStringList();
	for (auto const& file : recentProjectList)
	{
		if (!QFile::exists(file))
		{
			continue;
		}
		QFileInfo fileInfo(file);
		auto* recentpn = new QAction(this);
		recentpn->setText(fileInfo.dir().dirName() + "/" + fileInfo.fileName());
		recentpn->setData(fileInfo.absoluteFilePath());
		ui->menuRecent->addAction(recentpn);
		connect(recentpn, &QAction::triggered, this, &MainWindow::on_menuRecent_triggered);
	}

	ui->menuRecent->addSeparator();
	ui->menuRecent->addAction(ui->actionClear);
}

void MainWindow::LogMessage(QString const& message, spdlog::level::level_enum llvl)
{
	logger->log(llvl, message.toStdString());
}

QMap<QString, QString> MainWindow::GetFirmwareURLList() const
{
	//https://raw.githubusercontent.com/FalconChristmas/fpp-data/master/eepromList.json

	QString const url = "https://raw.githubusercontent.com/FalconChristmas/fpp-data/master/eepromList.json";

	QNetworkAccessManager manager;
	QNetworkRequest request(url);
	request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);
	QNetworkReply* response = manager.get(request);

	QElapsedTimer timer;
	timer.start();

	while (!response->isFinished())
	{
		if (timer.elapsed() >= (5 * 1000))
		{
			response->abort();
			return QMap<QString, QString>();
		}
		QCoreApplication::processEvents();
	}

	auto content = response->readAll();
	response->deleteLater();

	QJsonDocument loadDoc(QJsonDocument::fromJson(content));
	QJsonArray capeArray = loadDoc.array();
	QMap<QString, QString> capeList;
	for (auto const& cape : capeArray)
	{
		QJsonObject capeObj = cape.toObject();
		QString name = capeObj["cape"].toString();

		if (capeObj.contains("eeproms"))
		{
			QJsonArray eepromArray = capeObj["eeproms"].toArray();
			for (auto const& eeprom : eepromArray)
			{
				QJsonObject eepromObj = eeprom.toObject();
				QString ver = eepromObj["version"].toString();
				QString url = eepromObj["url"].toString();
				capeList.insert(name + "_" + ver, url);
			}
		}
	}

	return capeList;
}

void MainWindow::DownloadFirmware(QString const& name, QString const& url)
{
	QNetworkAccessManager manager;
	QNetworkRequest request(url);
	request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);
	QNetworkReply* response = manager.get(request);

	QElapsedTimer timer;
	timer.start();

	while (!response->isFinished())
	{
		if (timer.elapsed() >= (5 * 1000))
		{
			response->abort();
			return ;
		}
		QCoreApplication::processEvents();
	}

	auto content = response->readAll();
	response->deleteLater();

	QString folder = QStandardPaths::standardLocations(QStandardPaths::AppDataLocation).at(0);
    std::filesystem::create_directories(folder.toStdString());
	QString filePath = folder + "/" + name + ".bin";

	QFile file(filePath);
	if (!file.open(QFile::WriteOnly)) 
	{
		QMessageBox::information(this, tr("Unable to Save file"), file.errorString());
		return;
	}
	file.write(content);
	file.close();
	LoadEEPROM(filePath);
}
