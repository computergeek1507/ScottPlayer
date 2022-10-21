#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "./players/SequencePlayer.h"

#include "spdlog/spdlog.h"
#include "spdlog/common.h"

#include <QMainWindow>

#include <memory>
#include <filesystem>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }

class QListWidgetItem;
class QListWidget;
class QTableWidget;
class QSettings;
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

public Q_SLOTS:

    void on_actionSet_Show_Folder_triggered();
    void on_actionPlay_Sequence_triggered();
    void on_actionClose_triggered();

    void on_actionAbout_triggered();
    void on_actionOpen_Logs_triggered();
    
    void on_AddController(QString const&,QString const&,QString const&);

    void ClearListData();
    void UpdateStatus(QString const& message);
    void UpdatePlayback(QString const& sequenceName, int elapsedMS, int durationMS);
    void LogMessage(QString const& message , spdlog::level::level_enum llvl = spdlog::level::level_enum::debug);

private:
    QString FormatTime(int ticksMS) const;
    Ui::MainWindow *m_ui;

    std::shared_ptr<spdlog::logger> m_logger{ nullptr };
    std::unique_ptr<QSettings> m_settings{ nullptr };
    std::unique_ptr<SequencePlayer> m_player{ nullptr };
    QString m_appdir;
    QString m_showfolder;
};
#endif // MAINWINDOW_H
