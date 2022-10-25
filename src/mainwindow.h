#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "./players/SequencePlayer.h"
#include "./players/PlayListManager.h"

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
    void on_actionSave_triggered();
    void on_actionStop_Sequence_triggered();
    void on_actionClose_triggered();

    void on_actionAbout_triggered();
    void on_actionOpen_Logs_triggered();

    void on_pb_addPlaylist_clicked();
    void on_pb_deletePlaylist_clicked();

    void on_pb_addSequence_clicked();
    void on_pb_removeSequence_clicked();
    void on_pb_moveUp_clicked();
    void on_pb_moveDown_clicked();
    void on_pb_playSequence_clicked();

    void on_pb_addSchedule_clicked();
    void on_pb_editSchedule_clicked();
    void on_pb_deleteSchedule_clicked();
    void on_pb_sch_moveUp_clicked();
    void on_pb_sch_moveDown_clicked();

    void on_cb_playlists_currentIndexChanged( int index );
    
    void AddController_Received(QString const&,QString const&,QString const&);
    void RedrawPlaylist(int index);

    void SelectSequence(int index);

    void RedrawSchedule();

    void ClearListData();
    void UpdateStatus(QString const& message);
    void AddPlaylist(QString const& Playlist, int index);
    void UpdatePlayback(QString const& sequenceName, int elapsedMS, int durationMS);
    void LogMessage(QString const& message , spdlog::level::level_enum llvl = spdlog::level::level_enum::debug);

private:
    QString FormatTime(int ticksMS) const;
    QString GetFileName(QString const& path) const;

    Ui::MainWindow *m_ui;
    std::shared_ptr<spdlog::logger> m_logger{ nullptr };
    std::unique_ptr<QSettings> m_settings{ nullptr };
    std::unique_ptr<SequencePlayer> m_player{ nullptr };
    std::unique_ptr<PlayListManager> m_playlists{ nullptr };
    QString m_appdir;
    QString m_showfolder;
};
#endif // MAINWINDOW_H
