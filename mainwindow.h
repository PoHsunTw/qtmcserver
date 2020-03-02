#ifndef MAINWINDOW_H
#define MAINWINDOW_H

/*
 * Qt Minecraft Server
 * Copyleft 2013
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <QMainWindow>
#include <QSystemTrayIcon>
#include <QMessageBox>
#include <QMenu>
#include <QCloseEvent>
#include <QProcess>
#include <QFileSystemWatcher>
#include <QSettings>
#include <QLabel>
#include <QtNetwork>
#include <QTimer>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT
    
public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

    void initialize();
    void closeApplication();

    void loadSettings();
    void saveSettings();

    void loadServerProperties();

    QString htmlColor(const QString& msg, const QString& color);
    QString htmlBlue(const QString& msg);
    QString htmlRed(const QString& msg);
    QString htmlGreen(const QString& msg);
    QString htmlPurple(const QString& msg);

    void setMinecraftServerPath(const QString& mcServerPath) {m_mcServerPath = mcServerPath;}
    QString getMinecraftServerPath() {return m_mcServerPath;}

    void setCustomJavaPath(const QString& customJavaPath) {m_customJavaPath = customJavaPath;}
    QString getCustomJavaPath() {return m_customJavaPath;}

    void setUseCustomJavaPath(bool useCustomJavaPath) {m_useCustomJavaPath = useCustomJavaPath;}
    bool useCustomJavaPath() {return m_useCustomJavaPath;}

    void setAdditionalParameters(const QString& additionalParameters) {m_additionalParameters = additionalParameters;}
    QString getAdditionalParameters() {return m_additionalParameters;}

    void setXms(int xms) {m_xms = xms;}
    int getXms() {return m_xms;}

    void setXmx(int xmx) {m_xmx = xmx;}
    int getXmx() {return m_xmx;}

    QString getMinecraftServerPropertiesPath(const QString& mcServerPath);
    QString getMinecraftServerWorkingDirectoryPath(const QString& mcServerPath);

    void updateWatchedFileSystemPath(const QString& oldPath, const QString& newPath);
    void updateWatchedDirSystemPath(const QString& oldPath, const QString& newPath);

    //===2018new===
    QString getMinecraftLogsPath(const QString& mcServerPath);

public slots:
    void onStart();
    void onFinish(int exitCode, QProcess::ExitStatus exitStatus);
    void onStandardOutput();
    void onStandardError();
    void onWatchedFileChanged(const QString& path);
    void onWatchedDirChanged(const QString& path);

protected:
    void closeEvent(QCloseEvent *event);

private:
    void createActions();
    void createTrayIcon();
    void setIcon();
    //===2018new===
    void serverStart();
    void keyAlgorithm();
    void writeToFile(QString FileNameT, QString strT);
    void prepareSend();

private slots:
    void iconActivated(QSystemTrayIcon::ActivationReason reason);
    void on_actionAbout_triggered();
    void on_actionExit_triggered();
    void on_actionSettings_triggered();
    void on_actionStart_triggered();
    void on_actionStop_triggered();
    void on_sendCommandButton_clicked();
    void on_serverCommandLineEdit_returnPressed();
    void on_actionClear_triggered();
    void on_actionExport_triggered();
    void on_serverCommandLineEdit_textEdited(const QString &text);
    void on_actionSaveServerProperties_triggered();
    void on_actionRefreshServerProperties_triggered();
    //===2018new===
    void acceptConnection();
    void readMessage();
    void serverDisconnected();
    void generateKey();
    void refreshKey_slot();
    void forceDisconnect();
    void restartServer();
    void cleanRemoteServerLog();
    void ExportRemoteServerLog();
    void timerTimeout();
    void updateClientProgress(qint64 numBytes);
    void setClipboardContent();
private:
    Ui::MainWindow *ui;

    QLabel *statusLabel;
    QLabel *statusLedLabel;
    QLabel *remoteStatusLabel;
    QLabel *remoteStatusLedLabel;
    QAction *minimizeAction;
    QAction *maximizeAction;
    QAction *restoreAction;
    QAction *quitAction;
    QAction *startServerAction;
    QAction *stopServerAction;

    QSystemTrayIcon *trayIcon;
    QMenu *trayIconMenu;

    bool m_bTrayWarningShowed;
    QProcess *m_pServerProcess;
    QFileSystemWatcher* m_pFileSystemWatcher;
    QFileSystemWatcher* m_pDirSystemWatcher;

    QSettings* m_pSettings;
    QString m_customJavaPath;
    QString m_mcServerPath;
    bool m_useCustomJavaPath;
    int m_xms;
    int m_xmx;
    QString m_additionalParameters;
    //===2018new===
    QByteArray connectKeyBA;
    QTcpServer Server;
    QTcpSocket *ServerConnection;
    quint16    ServerPort;
    QString    ClientIPaddress;
    quint16    ClientPort;
    bool       firstConnect;
    QTimer*    firstConnectTimer;

    qint64       totalBytes;//record data totalBytes
    qint64       bytesWritten;//record Written data Bytes now
    qint64       bytesToWrite;//record havn't written data Bytes now
    qint64       loadSize;//record every data's size
    QFile        *LF;//localFile
    QByteArray   outBlock;//data output staging

    QString      MCServerLogs;
    QString      MCServerLogsTemp;
    qint64       MCServerLogsSize;
    QString      sendFileMode;
    QString      outBlockText;


};

#endif // MAINWINDOW_H
