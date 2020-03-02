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

#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "aboutdialog.h"
#include "settingsdialog.h"

#include <QFileDialog>
#include <QTextStream>
#include <QDebug>
#include <QCryptographicHash>
#include <QClipboard>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    m_bTrayWarningShowed = false;

    minimizeAction = 0;
    maximizeAction = 0;
    restoreAction = 0;
    quitAction = 0;
    trayIcon = 0;
    trayIconMenu = 0;

    m_pServerProcess = 0;
    m_pFileSystemWatcher = 0;
    m_pDirSystemWatcher = 0;

    m_pSettings = 0;
    m_useCustomJavaPath = false;
    m_mcServerPath = "";
    m_customJavaPath = "";
    m_xms = 512;
    m_xmx = 512;
    m_additionalParameters = "";

    statusLabel = 0;
    statusLedLabel = 0;
}

MainWindow::~MainWindow()
{
    if(m_pDirSystemWatcher)
    {
        delete m_pDirSystemWatcher;
        m_pDirSystemWatcher = 0;
    }

    if(m_pFileSystemWatcher)
    {
        delete m_pFileSystemWatcher;
        m_pFileSystemWatcher = 0;
    }

    if(m_pSettings)
    {
        delete m_pSettings;
        m_pSettings = 0;
    }

    delete ui;
}

QString MainWindow::htmlColor(const QString &msg, const QString &color)
{
    return QString("<font color=\"%1\">%2</font>").arg(color).arg(msg);
}

QString MainWindow::htmlBlue(const QString &msg)
{
    return htmlColor(msg, "blue");
}

QString MainWindow::htmlRed(const QString &msg)
{
    return htmlColor(msg, "red");
}

QString MainWindow::htmlGreen(const QString &msg)
{
    return htmlColor(msg, "green");
}

QString MainWindow::htmlPurple(const QString &msg)
{
    return htmlColor(msg, "purple");
}

void MainWindow::initialize()
{
    createActions();
    createTrayIcon();
    setIcon();

    statusLedLabel = new QLabel;
    if(statusLedLabel)
    {
        statusBar()->addWidget(statusLedLabel);
        statusLedLabel->setPixmap(QPixmap("://images/led-red.png"));
    }

    statusLabel = new QLabel;
    if(statusLabel)
    {
        statusBar()->addWidget(statusLabel);
        statusLabel->setText(tr("Minecraft Server: Stopped"));
    }

    //===2018new===

    remoteStatusLedLabel = new QLabel;
    if(remoteStatusLedLabel)
    {
        statusBar()->addWidget(remoteStatusLedLabel);
        remoteStatusLedLabel->setPixmap(QPixmap("://images/led-red.png"));
    }
    remoteStatusLabel = new QLabel;
    if(remoteStatusLabel)
    {
        statusBar()->addWidget(remoteStatusLabel);
        remoteStatusLabel->setText(tr("Remote Server: Disconnected"));
    }

    //=============
    ui->actionStart->setEnabled(true);
    ui->actionStop->setEnabled(false);
    ui->actionSettings->setEnabled(true);
    ui->serverPropertiesTextEdit->setEnabled(true);
    ui->sendCommandButton->setEnabled(false);
    ui->actionSaveServerProperties->setEnabled(false);

    if(trayIcon)
    {
        trayIcon->show();
    }

    m_pServerProcess = new QProcess(this);

    connect( m_pServerProcess, SIGNAL(started()), SLOT(onStart()) );
    connect( m_pServerProcess, SIGNAL(finished(int,QProcess::ExitStatus)), SLOT(onFinish(int,QProcess::ExitStatus)) );
    connect( m_pServerProcess, SIGNAL(readyReadStandardOutput()), SLOT(onStandardOutput()) );
    connect( m_pServerProcess, SIGNAL(readyReadStandardError()), SLOT(onStandardError()) );

    m_pFileSystemWatcher = new QFileSystemWatcher(this);
    connect( m_pFileSystemWatcher, SIGNAL(fileChanged(QString)), SLOT(onWatchedFileChanged(QString)) );

    m_pDirSystemWatcher = new QFileSystemWatcher(this);
    connect( m_pDirSystemWatcher, SIGNAL(directoryChanged(QString)), SLOT(onWatchedDirChanged(QString)) );

    m_pSettings = new QSettings(QSettings::IniFormat, QSettings::UserScope, "Qt Minecraft Server", "qtmcserver", this);

    loadSettings();

    if(m_mcServerPath.isEmpty())
    {
        on_actionSettings_triggered();
    }

    loadServerProperties();

    if(!m_mcServerPath.isEmpty())
    {
        updateWatchedFileSystemPath("", getMinecraftServerPropertiesPath(m_mcServerPath));
        updateWatchedDirSystemPath("", getMinecraftServerWorkingDirectoryPath(m_mcServerPath));
    }
    //===2018new===
    ui->CopyButton->setEnabled(false);
    ui->refreshKeyButton->setEnabled(false);
    serverStart();
    connect(ui->generateKeyButton,SIGNAL(clicked()),this,SLOT(generateKey()));
    connect(ui->forceDisconnectButton,SIGNAL(clicked()),this,SLOT(forceDisconnect()));
    connect(ui->RestartServerButton,SIGNAL(clicked()),this,SLOT(restartServer()));
    connect(ui->exportLogButton,SIGNAL(clicked()),this,SLOT(ExportRemoteServerLog()));
    connect(ui->cleanLogButton,SIGNAL(clicked()),this,SLOT(cleanRemoteServerLog()));
    connect(ui->refreshKeyButton,SIGNAL(clicked()),this,SLOT(refreshKey_slot()));
    connect(ui->CopyButton, SIGNAL(clicked()), this, SLOT(setClipboardContent()));
}

void MainWindow::closeApplication()
{
    saveSettings();
    qApp->quit();
}

void MainWindow::loadSettings()
{
    if(m_pSettings)
    {
        QString strUseCustomJavaPath = m_pSettings->value("Settings/UseCustomJavaPath", "no").toString();
        m_useCustomJavaPath = (strUseCustomJavaPath == "yes") ? true : false;

        m_customJavaPath = m_pSettings->value("Settings/CustomJavaPath", "").toString();
        m_mcServerPath = m_pSettings->value("Settings/MinecraftServerPath", "").toString();
        m_xms =  m_pSettings->value("Settings/Xms", "512").toInt();
        m_xmx =  m_pSettings->value("Settings/Xmx", "512").toInt();
        m_additionalParameters = m_pSettings->value("Settings/AdditionalParameters", "").toString();
    }
}

void MainWindow::saveSettings()
{
    if(m_pSettings)
    {
        m_pSettings->setValue("Settings/UseCustomJavaPath", m_useCustomJavaPath ? "yes" : "no");
        m_pSettings->setValue("Settings/CustomJavaPath", m_customJavaPath);
        m_pSettings->setValue("Settings/MinecraftServerPath", m_mcServerPath);
        m_pSettings->setValue("Settings/Xms", m_xms);
        m_pSettings->setValue("Settings/Xmx", m_xmx);
        m_pSettings->setValue("Settings/AdditionalParameters", m_additionalParameters);
    }
}

void MainWindow::loadServerProperties()
{
    if(!m_mcServerPath.isEmpty())
    {
        if(m_pServerProcess && (m_pServerProcess->state() == QProcess::NotRunning))
        {
            ui->actionSaveServerProperties->setEnabled(true);
        }

        ui->serverPropertiesTextEdit->clear();

        QFile file(getMinecraftServerPropertiesPath(m_mcServerPath));
        if(!file.open(QIODevice::ReadOnly))
        {
            return;
        }

        QTextStream in(&file);

        QString contents = in.readAll();
        ui->serverPropertiesTextEdit->setText(contents);

        file.close();
    }
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    if (trayIcon && trayIcon->isVisible())
    {
        if(!m_bTrayWarningShowed)
        {
            QMessageBox::information(this, tr("Qt Minecraft Server"),
                                 tr("Qt Minecraft Server will keep running in the "
                                    "system tray. To terminate Qt Minecraft Server, "
                                    "choose <b>Exit</b> in the context menu "
                                    "of the system tray entry."));
            m_bTrayWarningShowed = true;
        }

        hide();
        event->ignore();
    }
}

void MainWindow::createActions()
{
    minimizeAction = new QAction(tr("Mi&nimize"), this);
    connect(minimizeAction, SIGNAL(triggered()), this, SLOT(hide()));

    maximizeAction = new QAction(tr("Ma&ximize"), this);
    connect(maximizeAction, SIGNAL(triggered()), this, SLOT(showMaximized()));

    restoreAction = new QAction(tr("&Restore"), this);
    connect(restoreAction, SIGNAL(triggered()), this, SLOT(showNormal()));

    quitAction = new QAction(tr("E&xit"), this);
    connect(quitAction, SIGNAL(triggered()), this, SLOT(on_actionExit_triggered()));
}

void MainWindow::createTrayIcon()
{
    trayIconMenu = new QMenu(this);

    if(trayIconMenu)
    {
        trayIconMenu->addAction(minimizeAction);
        trayIconMenu->addAction(maximizeAction);
        trayIconMenu->addAction(restoreAction);
        trayIconMenu->addSeparator();
        trayIconMenu->addAction(ui->actionStart);
        trayIconMenu->addAction(ui->actionStop);
        trayIconMenu->addSeparator();
        trayIconMenu->addAction(quitAction);

        trayIcon = new QSystemTrayIcon(this);

        if(trayIcon)
        {
            trayIcon->setToolTip("Qt Minecraft Server");
            trayIcon->setContextMenu(trayIconMenu);
        }
    }
}

void MainWindow::setIcon()
{
    QIcon icon = QIcon("://images/qtmcserver-16x16.png");
    trayIcon->setIcon(icon);
}

void MainWindow::iconActivated(QSystemTrayIcon::ActivationReason reason)
{
    switch (reason) {
        case QSystemTrayIcon::Trigger:
        case QSystemTrayIcon::DoubleClick:
            showNormal();
            break;
        case QSystemTrayIcon::MiddleClick:
            break;
        default:
            ;
    }
}

void MainWindow::on_actionAbout_triggered()
{
    AboutDialog* aboutDlg = new AboutDialog(this);

    if(aboutDlg)
    {
        aboutDlg->initialize();
        aboutDlg->exec();

        delete aboutDlg;
        aboutDlg = 0;
    }
}

void MainWindow::on_actionExit_triggered()
{
    if(ui->forceDisconnectButton->isEnabled()){
        forceDisconnect();
    }
    if(m_pServerProcess)
    {
        if(m_pServerProcess->state() == QProcess::Running)
        {
            on_actionStop_triggered();
            m_pServerProcess->waitForFinished();
        }

        if(m_pServerProcess->state() == QProcess::NotRunning)
        {
            closeApplication();
        }
    }
    else
    {
        closeApplication();
    }
}

void MainWindow::on_actionSettings_triggered()
{
    SettingsDialog* settingsDlg = new SettingsDialog(this);

    if(settingsDlg)
    {
        settingsDlg->setUseCustomJavaPath(m_useCustomJavaPath);
        settingsDlg->setCustomJavaPath(m_customJavaPath);
        settingsDlg->setMinecraftServerPath(m_mcServerPath);
        settingsDlg->setXms(m_xms);
        settingsDlg->setXmx(m_xmx);
        settingsDlg->setAdditionalParameters(m_additionalParameters);

        settingsDlg->initialize();

        if(settingsDlg->exec() == QDialog::Accepted)
        {
            updateWatchedFileSystemPath( getMinecraftServerPropertiesPath(m_mcServerPath),
                                         getMinecraftServerPropertiesPath(settingsDlg->getMinecraftServerPath()));

            updateWatchedDirSystemPath( getMinecraftServerWorkingDirectoryPath(m_mcServerPath),
                                        getMinecraftServerWorkingDirectoryPath(settingsDlg->getMinecraftServerPath()));

            m_mcServerPath = settingsDlg->getMinecraftServerPath();
            m_customJavaPath = settingsDlg->getCustomJavaPath();
            m_useCustomJavaPath = settingsDlg->useCustomJavaPath();
            m_xms = settingsDlg->getXms();
            m_xmx = settingsDlg->getXmx();
            m_additionalParameters = settingsDlg->getAdditionalParameters();

            loadServerProperties();
        }

        delete settingsDlg;
        settingsDlg = 0;
    }
}

QString MainWindow::getMinecraftServerPropertiesPath(const QString& mcServerPath)
{
    QString mcServerPropertiesPath = "";

    if(!mcServerPath.isEmpty())
    {
        QFileInfo mcServerFileInfo = QFileInfo(mcServerPath);
        QString workingDir = mcServerFileInfo.absolutePath();

        mcServerPropertiesPath = workingDir + QString("/server.properties");
    }

    return mcServerPropertiesPath;
}

QString MainWindow::getMinecraftServerWorkingDirectoryPath(const QString& mcServerPath)
{
    QString mcServerWorkingDirectoryPath = "";

    if(!mcServerPath.isEmpty())
    {
        QFileInfo mcServerFileInfo = QFileInfo(mcServerPath);
        mcServerWorkingDirectoryPath = mcServerFileInfo.absolutePath();
    }

    return mcServerWorkingDirectoryPath;
}

void MainWindow::updateWatchedFileSystemPath(const QString& oldPath, const QString& newPath)
{
    if(m_pFileSystemWatcher)
    {
        if(!oldPath.isEmpty())
        {
            if(m_pFileSystemWatcher->files().contains(oldPath))
            {
                m_pFileSystemWatcher->removePath(oldPath);
            }
        }

        if(!newPath.isEmpty())
        {
            if(QFile::exists(newPath))
            {
                if(!m_pFileSystemWatcher->files().contains(newPath))
                {
                    m_pFileSystemWatcher->addPath(newPath);
                }
            }
        }
    }
}

void MainWindow::updateWatchedDirSystemPath(const QString &oldPath, const QString &newPath)
{
    if(m_pDirSystemWatcher)
    {
        if(!oldPath.isEmpty())
        {
            if(m_pDirSystemWatcher->directories().contains(oldPath))
            {
                m_pDirSystemWatcher->removePath(oldPath);
            }
        }

        if(!newPath.isEmpty())
        {
            if(!m_pDirSystemWatcher->directories().contains(newPath))
            {
                m_pDirSystemWatcher->addPath(newPath);
            }
        }
    }
}

void MainWindow::on_actionStart_triggered()
{
    if(m_mcServerPath.isEmpty())
    {
        QMessageBox::information(this, tr("Qt Minecraft Server"),
                                 tr("No Minecraft Server File available!\nPlease select a Minecraft Server File at Qt Minecraft Server Settings."));

        on_actionSettings_triggered();

        return;
    }

    if(m_pServerProcess)
    {
        QFileInfo mcServerFileInfo = QFileInfo(m_mcServerPath);
        QString workingDir = mcServerFileInfo.absolutePath();
        QString mcServerFile = mcServerFileInfo.fileName();
        QStringList mcServerFileTypeList = mcServerFile.split('.');
        QString mcServerFileType =  mcServerFileTypeList.rbegin()[0];

        m_pServerProcess->setWorkingDirectory(workingDir);

        QStringList arguments;
        if(mcServerFileType=="bat"){
            arguments.append("/c");
            arguments.append(mcServerFile);
            ui->serverLogTextEdit->append(htmlBlue(tr("&gt;&gt; Starting Java VM (bat) in Working Directory: %1...")
                                                    .arg(QDir::toNativeSeparators(workingDir))));
            ui->serverLogTextEdit->append(htmlBlue(tr("&gt;&gt; cmd.exe %1").arg(arguments.join(" "))));
            m_pServerProcess->start("cmd.exe",arguments, QIODevice::ReadWrite | QIODevice::Unbuffered);
            if(!m_pServerProcess->waitForStarted())
            {
                ui->serverLogTextEdit->append(htmlRed(tr("&gt;&gt; Unable to start bat.")));
            }
        }else{


            if(m_xms > 0)
            {
                arguments.append(QString("-Xms%1M").arg(QString::number(m_xms)));
                on_actionSaveServerProperties_triggered();

            }

            if(m_xmx > 0)
            {
                arguments.append(QString("-Xmx%1M").arg(QString::number(m_xmx)));
            }

            arguments.append("-jar");
            arguments.append(mcServerFile);
            arguments.append("nogui");

            if(!m_additionalParameters.isEmpty())
            {
                arguments.append(m_additionalParameters);
            }

            on_actionSaveServerProperties_triggered();

            ui->serverLogTextEdit->append(htmlBlue(tr("&gt;&gt; Starting Java VM in Working Directory: %1...")
                                                   .arg(QDir::toNativeSeparators(workingDir))));

            if(m_useCustomJavaPath)
            {
                ui->serverLogTextEdit->append(htmlBlue(tr("&gt;&gt; %1 %2").arg(QDir::toNativeSeparators(m_customJavaPath))
                                                       .arg(arguments.join(" "))));

                m_pServerProcess->start(m_customJavaPath, arguments, QIODevice::ReadWrite | QIODevice::Unbuffered);
            }
            else
            {
                ui->serverLogTextEdit->append(htmlBlue(tr("&gt;&gt; java %1").arg(arguments.join(" "))));
                m_pServerProcess->start("java", arguments, QIODevice::ReadWrite | QIODevice::Unbuffered);
            }

            if(!m_pServerProcess->waitForStarted())
            {
                ui->serverLogTextEdit->append(htmlRed(tr("&gt;&gt; Unable to start Java VM.")));
            }
        }
    }
}

void MainWindow::onStart()
{
    ui->serverLogTextEdit->append(htmlBlue(tr("&gt;&gt; Starting Minecraft Server...")));

    ui->actionStart->setEnabled(false);
    ui->actionStop->setEnabled(true);
    ui->actionSettings->setEnabled(false);
    ui->serverPropertiesTextEdit->setEnabled(false);
    ui->actionSaveServerProperties->setEnabled(false);

    statusLabel->setText(tr("Minecraft Server: Running"));
    statusLedLabel->setPixmap(QPixmap("://images/led-green.png"));
}

void MainWindow::onFinish(int exitCode, QProcess::ExitStatus exitStatus)
{
    if((exitStatus == QProcess::NormalExit) && (exitCode ==  0))
    {
        ui->serverLogTextEdit->append(htmlBlue(tr("&gt;&gt; Minecraft Server stopped normally with exit code: %1").arg(exitCode)));
    }
    else if((exitStatus == QProcess::NormalExit) && (exitCode ==  1))
    {
        ui->serverLogTextEdit->append(htmlRed(tr("&gt;&gt; Minecraft Server killed and exited with exit code: %1").arg(exitCode)));
    }
    else if(exitStatus == QProcess::CrashExit)
    {
        ui->serverLogTextEdit->append(htmlRed(tr("&gt;&gt; Minecraft Server crashed!")));
    }

    ui->actionStart->setEnabled(true);
    ui->actionStop->setEnabled(false);
    ui->actionSettings->setEnabled(true);
    ui->serverPropertiesTextEdit->setEnabled(true);
    ui->actionSaveServerProperties->setEnabled(true);

    statusLabel->setText(tr("Minecraft Server: Stopped"));
    statusLedLabel->setPixmap(QPixmap("://images/led-red.png"));
}

void MainWindow::onStandardOutput()
{
    QByteArray baOutput = m_pServerProcess->readAllStandardOutput();
    QString str;

    if (!baOutput.isEmpty())
    {
        str = QString::fromUtf8(baOutput).trimmed();

        if(!str.isEmpty())
            ui->serverLogTextEdit->append(str);
    }
}

void MainWindow::onStandardError()
{
    QByteArray baError = m_pServerProcess->readAllStandardError();
    QString str;

    if (!baError.isEmpty())
    {
        str = QString(baError).trimmed();

        if(!str.isEmpty())
        {
            ui->serverLogTextEdit->append(str);
        }
    }
}

void MainWindow::onWatchedFileChanged(const QString &path)
{
    if(!m_mcServerPath.isEmpty())
    {
        if(path == getMinecraftServerPropertiesPath(m_mcServerPath))
        {
            loadServerProperties();
        }
    }
}

void MainWindow::onWatchedDirChanged(const QString &path)
{
    if(!m_mcServerPath.isEmpty())
    {
        if(path == getMinecraftServerWorkingDirectoryPath(m_mcServerPath))
        {
            QString mcServerPropertiesPath = getMinecraftServerPropertiesPath(m_mcServerPath);

            if(QFile::exists(mcServerPropertiesPath))
            {
                updateWatchedFileSystemPath(mcServerPropertiesPath, mcServerPropertiesPath);
            }
        }
    }
}

void MainWindow::on_actionStop_triggered()
{
    if(m_pServerProcess)
    {
        if(m_pServerProcess->state() == QProcess::Running)
        {
            ui->serverLogTextEdit->append(htmlBlue(tr("&gt;&gt; Stopping Minecraft Server...")));

            if(m_pServerProcess->isWritable())
            {
                QByteArray command = (QString("stop") + QString("\n")).toLatin1();

                m_pServerProcess->write(command);
                m_pServerProcess->waitForBytesWritten();
            }
        }
    }
}

void MainWindow::on_sendCommandButton_clicked()
{
    if(ui->serverCommandLineEdit->text().isEmpty())
        return;

    if(m_pServerProcess)
    {
        if(m_pServerProcess->state() == QProcess::Running)
        {
            if(m_pServerProcess->isWritable())
            {
                ui->serverLogTextEdit->append(htmlGreen(QString("&lt;&lt; ") + ui->serverCommandLineEdit->text()));

                if(ui->serverCommandLineEdit->text().trimmed() == "stop")
                {
                    ui->serverLogTextEdit->append(htmlBlue(tr("&gt;&gt; Stopping Minecraft Server...")));
                }

                QByteArray command = (ui->serverCommandLineEdit->text() + QString("\n")).toLatin1();

                m_pServerProcess->write(command);
                m_pServerProcess->waitForBytesWritten();

                ui->serverCommandLineEdit->clear();
            }
        }
    }
}

void MainWindow::on_serverCommandLineEdit_returnPressed()
{
    on_sendCommandButton_clicked();
}

void MainWindow::on_actionClear_triggered()
{
    ui->serverLogTextEdit->clear();
}

void MainWindow::on_actionExport_triggered()
{
    QString fileName = QFileDialog::getSaveFileName(this, tr("Export Console Log"),
                               "mcserverlog.txt",
                               tr("Text Files (*.txt)"));
    if(!fileName.isEmpty())
    {
        QFile outfile;
        outfile.setFileName(fileName);

        if(outfile.open(QIODevice::WriteOnly | QIODevice::Text))
        {
            QTextStream out(&outfile);
            out << ui->serverLogTextEdit->toPlainText() << endl;
            outfile.close();
        }
    }
}

void MainWindow::on_actionSaveServerProperties_triggered()
{
    if(!m_mcServerPath.isEmpty())
    {
        QFileInfo mcServerFileInfo = QFileInfo(m_mcServerPath);
        QString workingDir = mcServerFileInfo.absolutePath();
        qDebug()<<workingDir;
        QFile outfile;
        outfile.setFileName(workingDir + QString("/server.properties"));

        if(outfile.open(QIODevice::WriteOnly | QIODevice::Text))
        {
            QTextStream out(&outfile);
            out << ui->serverPropertiesTextEdit->toPlainText();

            outfile.close();
        }
    }
}

void MainWindow::on_actionRefreshServerProperties_triggered()
{
    QString mcServerPropertiesPath = getMinecraftServerPropertiesPath(m_mcServerPath);
    updateWatchedFileSystemPath(mcServerPropertiesPath, mcServerPropertiesPath);

    loadServerProperties();
}

void MainWindow::on_serverCommandLineEdit_textEdited(const QString &text)
{
    ui->sendCommandButton->setEnabled(!text.isEmpty());
}

//===2018new===
void MainWindow::keyAlgorithm(){
    QString dateNow = QDateTime::currentDateTime().toString("yyyy/MM/dd-HH:mm");
    QByteArray dateBA = dateNow.toLatin1();
    connectKeyBA = QCryptographicHash::hash(dateBA,QCryptographicHash::Sha3_512);
    QString keyS = QString::fromLatin1(connectKeyBA.data());
    //qDebug()<<"dateBA:"<<dateBA;
    //qDebug()<<"connectKeyBA:"<<connectKeyBA;
    //qDebug()<<"keyS:"<<keyS.toLatin1();
}
QString MainWindow::getMinecraftLogsPath(const QString& mcServerPath)
{
    QString mcServerLogsPath = "";

    if(!mcServerPath.isEmpty())
    {
        QFileInfo mcServerFileInfo = QFileInfo(mcServerPath);
        QString workingDir = mcServerFileInfo.absolutePath();

        mcServerLogsPath = workingDir + QString("/logs/latest.log");
    }

    return mcServerLogsPath;
}

void MainWindow::writeToFile(QString FileNameT, QString strT){
    QFile FileT(FileNameT);
    //開啟檔案
    if(!FileT.open(QFile::WriteOnly|QFile::Text)){
        qDebug()<<"could not open the file for write";
        return;
    }
    QTextStream out(&FileT);//文字流物件銜接檔案物件(QFile)
    out<<strT;
    FileT.flush();
    FileT.close();
}
//---SLOT---
void MainWindow::serverStart(){
    ui->forceDisconnectButton->setText("Stop Listening");
    connect(&Server,SIGNAL(newConnection()),this,SLOT(acceptConnection()));
    //TODO ServerPort change to lineEdit
    ServerPort  =  7777;
    Server.listen(QHostAddress::AnyIPv4,ServerPort);
    QString remoteLog = QDateTime::currentDateTime().toString("yyyy/MM/dd hh:mm:ss ");
    remoteLog.append(htmlGreen("========RemoteServerStarted!!========"));
    ui->connectionLogText->append(remoteLog);
    remoteStatusLabel->setText(tr("Remote Server: Listening"));
    remoteStatusLedLabel->setPixmap(QPixmap("://images/led-orange.png"));
    refreshKey_slot();
}

void MainWindow::acceptConnection(){//accept&check verification
    qDebug()<<"[Pit]new connection";
    firstConnect = true;
    firstConnectTimer = new QTimer;
    firstConnectTimer->start(3*1000);
    connect(firstConnectTimer,SIGNAL(timeout()),this,SLOT(timerTimeout()));
//    ui->RestartServerButton->setEnabled(true);
//    ui->exportLogButton->setEnabled(true);
//    ui->cleanLogButton->setEnabled(true);
//    ui->forceDisconnectButton->setEnabled(true);
    ui->forceDisconnectButton->setText("Force Disconnect");
    remoteStatusLabel->setText(tr("Remote Server: Connected!"));
    remoteStatusLedLabel->setPixmap(QPixmap("://images/led-green.png"));
    ServerConnection = Server.nextPendingConnection();
    Server.close();//關閉其他客戶端連線
    //以上是從TCP server物件取得已接受連線
    QString remoteLog = QDateTime::currentDateTime().toString("yyyy/MM/dd hh:mm:ss ");
    remoteLog.append(htmlBlue("========RemoteServerConnect!!========"));
    ui->connectionLogText->append(remoteLog);
    connect(ServerConnection,SIGNAL(readyRead()),this,SLOT(readMessage()));
    connect(ServerConnection,SIGNAL(disconnected()),this,SLOT(serverDisconnected()));
    ClientIPaddress = ServerConnection->peerAddress().toString();
    ClientPort = ServerConnection->peerPort();
    //---
    remoteLog = QDateTime::currentDateTime().toString("yyyy/MM/dd hh:mm:ss ");
    remoteLog.append(htmlBlue("Remote Client Connect from:"+ClientIPaddress+
                     ":"+QString::number(ClientPort)));
    ui->connectionLogText->append(remoteLog);
    //---
}

void MainWindow::readMessage(){
    qDebug()<<"[PIT]readMessage";
    //---read---
    QByteArray ServerRead = ServerConnection->read(ServerConnection->bytesAvailable());
    QString strIN = QString::fromLatin1(ServerRead,ServerRead.size());
    //qDebug()<<"strINsize"<<strIN.size()<<"readsize"<<ServerRead.size();
    //---analysis---
    QStringList strList = strIN.split('|');
    QString strType = strList[0];
    QString strCommand = strList[1];
//    qDebug()<<"listsize"<<strList.size();
//    qDebug()<<"strIN"<<strIN;
//    qDebug()<<"strINBA"<<strCommand.toLatin1();
//    qDebug()<<"self"<<connectKeyBA;
    //------
    QString remoteLog = QDateTime::currentDateTime().toString("yyyy/MM/dd hh:mm:ss ");
    if(strList.size()!=2){
        if(strList.size() < 2){
            qDebug()<<"strList.size amount to small";
            remoteLog.append(htmlRed("\"error format\""));
            qDebug()<<remoteLog;
            ui->connectionLogText->append(remoteLog);
            return;
        }else if(strList.size() > 2){
            qDebug()<<"strList.size amount to many";
            for(int i=2;i<strList.size();i++){
                strCommand.append('|');
                strCommand.append(strList[i]);
            }
        }
    }
    if(!firstConnect && strType == "key"){
        qDebug()<<"[PIT]not first time connect";
    }
    if(firstConnect){
        qDebug()<<"[PIT]first time connect";
        disconnect(firstConnectTimer,SIGNAL(timeout()),this,SLOT(timerTimeout()));
        firstConnectTimer->destroyed();
        firstConnect =false;
        if(strType == "key"){
            if(connectKeyBA == strCommand.toLatin1()){
                remoteLog.append(htmlBlue("========Verification Succesful========"));
                ServerConnection->write("remote|success");
                ServerConnection->waitForBytesWritten();
            }else{
                remoteLog.append(htmlPurple("Reason : Verification fail(wrong key)"));
                ServerConnection->write("remote|Verification fail|wrong key");
                ServerConnection->waitForBytesWritten();
                restartServer();
                //ServerConnection->disconnectFromHost();
            }
        }else{
            remoteLog.append(htmlPurple("Reason : Verification fail(didn't found any key)"));
            ServerConnection->write("remote|Verification fail|didn't found any key");
            ServerConnection->waitForBytesWritten();
            restartServer();
            //ServerConnection->disconnectFromHost();
        }
    }else if(strType == "button"){
        if(strCommand == "start"){
            if(ui->actionStart->isEnabled()) on_actionStart_triggered();
            remoteLog.append("PushButton\"start\"");
        }else if(strCommand == "stop"){
            if(ui->actionStop->isEnabled()) {
                on_actionStop_triggered();
                ui->actionStop->setEnabled(false);
            }
            remoteLog.append("PushButton\"stop\"");
        }
    }else if(strType == "command"){
        ui->serverCommandLineEdit->text().toLatin1();
        if(strCommand=="texttest"){
            return;
        }
        if(statusLabel->text()==QString::fromLatin1("Minecraft Server: Stopped")){
            qDebug()<<"sendCommandButtonN";
            ServerConnection->write("reason|Send Command Error Occur!!Reason : Server isn't running.");
            ServerConnection->waitForBytesWritten();
        }
        else{
            qDebug()<<"sendCommandButtonY";
            ui->serverCommandLineEdit->setText(QString::fromUtf8(strCommand.toLatin1()));
            on_sendCommandButton_clicked();
        }
        remoteLog.append("ReceiveCommand");
        remoteLog.append("\""+strCommand+"\"");
    }else if(strType == "file"){
        prepareSend();
        remoteLog.append("PushButton\"get logs\" size:"+QString::number(totalBytes/1024.0)+"KB");
        ServerConnection->waitForBytesWritten();
    }else if(strType == "mcServerStatus"){
        ServerConnection->write("mcServerStatus|"+QDateTime::currentDateTime().toString("yyyy/MM/dd hh:mm:ss ").toLatin1()+
                                statusLabel->text().toLatin1());
        remoteLog.append("PushButton\"get mcServerStatus\"");
        ServerConnection->waitForBytesWritten();
    }else if(strType == "mcServerLogs"){
        if(strCommand==""){
            if(!ui->serverLogTextEdit->document()->isEmpty()){
                MCServerLogsTemp = ui->serverLogTextEdit->document()->toHtml();
                MCServerLogsSize = MCServerLogsTemp.size();
                QString strSend = "mcServerLogs|"+QString::number(MCServerLogsSize);
                ServerConnection->write(strSend.toLatin1());
                ServerConnection->waitForBytesWritten();
            }else{
                ServerConnection->write("reason|mcServerLogs:Didn't Found Any Logs.");
                ServerConnection->waitForBytesWritten();
            }
        }else if(strCommand=="start"){
            qDebug()<<"[PIT]MCServerLogs:Start";

//            QDataStream sendOut(&outBlock,QIODevice::WriteOnly);
//            sendOut.setVersion(QDataStream::Qt_4_6);//設定QT資料格式版本號
//            sendOut<<MCServerLogsTemp;
//            outBlock.resize(0);//清空outBlock


            QTextStream  textSendOut(&outBlockText,QIODevice::WriteOnly);
            textSendOut<<MCServerLogsTemp;
            qDebug()<<"write bits"<<ServerConnection->write(outBlockText.toLatin1());
            ServerConnection->waitForBytesWritten();
            qDebug()<<"totalBits"<<MCServerLogsSize<<"checkBits"<<outBlockText.size();
            outBlockText.resize(0);
            MCServerLogsTemp = "";
            MCServerLogsSize = 0;

            //ServerConnection->write(MCServerLogsTemp.toLatin1());
            //ServerConnection->waitForBytesWritten();
        }else if(strCommand=="finished"){
            qDebug()<<"[PIT]MCServerLogs:finished";
            remoteLog.append(htmlGreen("successfully send logs"));
            ui->connectionLogText->append(remoteLog);
        }
        return;
//        if(!ui->serverLogTextEdit->document()->isEmpty()){
//            ui->serverLogTextEdit->moveCursor( QTextCursor::End, QTextCursor::MoveAnchor );
//            ui->serverLogTextEdit->moveCursor( QTextCursor::StartOfLine, QTextCursor::MoveAnchor );
//            ui->serverLogTextEdit->moveCursor( QTextCursor::EndOfLine, QTextCursor::KeepAnchor );
//            QString lastLineStr = ui->serverLogTextEdit->textCursor().selectedText();
//            ui->serverLogTextEdit->moveCursor( QTextCursor::End, QTextCursor::MoveAnchor );
//            ui->serverLogTextEdit->setTextColor(QColor(0,0,0));
//            ServerConnection->write("mcServerLogs|"+htmlBlue(lastLineStr).toLatin1());
//            ServerConnection->waitForBytesWritten();
//        }else{

//        }

    }else if(strType == "mcLogsUpdate"){
        if(MCServerLogsSize){
            qint64 MCServerLogsSizeNow = ui->serverLogTextEdit->document()->toHtml().size();
            if(MCServerLogsSizeNow - MCServerLogsSize>0){
                MCServerLogs = ui->serverLogTextEdit->document()->toHtml();
                QString MCServerLogsDiff = MCServerLogs;
                MCServerLogsDiff.remove(0,MCServerLogsSize-1);
                MCServerLogsSize = MCServerLogsSizeNow;
                ServerConnection->write("mcLogsUpdate|"+MCServerLogsDiff.toLatin1());
                ServerConnection->waitForBytesWritten();
                return;
            }
        }
    }else{
        remoteLog.append(htmlRed("\"error format\"->")+htmlPurple(strIN));
    }
    //qDebug()<<remoteLog;
    ui->connectionLogText->append(remoteLog);
    ui->connectionLogText->setTextColor(QColor(0,0,0));
}
void MainWindow::serverDisconnected(){
    qDebug()<<"[PIT]serverDisconnected";
    ui->forceDisconnectButton->setText("Stop Listening");
    disconnect(ServerConnection,SIGNAL(readyRead()),this,SLOT(readMessage()));
    disconnect(ServerConnection,SIGNAL(disconnected()),this,SLOT(serverDisconnected()));
    QString remoteLog = QDateTime::currentDateTime().toString("yyyy/MM/dd hh:mm:ss ");
    remoteLog.append(htmlRed("========RemoteServerDisconnect!!========"));
    ui->connectionLogText->append(remoteLog);
    remoteStatusLabel->setText(tr("Remote Server: Listening"));
    remoteStatusLedLabel->setPixmap(QPixmap("://images/led-orange.png"));
    Server.listen(QHostAddress::AnyIPv4,ServerPort);
}

void MainWindow::generateKey(){
    //qDebug()<<"connectKeyBA:"<<connectKeyBA;
    ui->CopyButton->setEnabled(true);
    ui->refreshKeyButton->setEnabled(true);
    ui->keyLineEdit->setText(QString::fromLatin1(connectKeyBA));
    QString remoteLog = QDateTime::currentDateTime().toString("yyyy/MM/dd hh:mm:ss ");
    remoteLog.append(htmlColor("key is:"+htmlColor(QString::fromLatin1(connectKeyBA),"orange"),"black"));
    ui->connectionLogText->append(remoteLog);
}

void MainWindow::refreshKey_slot(){
    keyAlgorithm();
    generateKey();
}

void MainWindow::forceDisconnect(){
    remoteStatusLabel->setText(tr("Remote Server: Disconnected"));
    remoteStatusLedLabel->setPixmap(QPixmap("://images/led-red.png"));
    QString remoteLog = QDateTime::currentDateTime().toString("yyyy/MM/dd hh:mm:ss ");
    if(ui->forceDisconnectButton->text()=="Force Disconnect"){
        disconnect(firstConnectTimer,SIGNAL(timeout()),this,SLOT(timerTimeout()));
        ServerConnection->disconnectFromHost();
        remoteLog.append(htmlRed("========RemoteServerForceDisconnect!!========"));
    }else{

        remoteLog.append(htmlRed("========RemoteServerStopListening!!========"));
    }
    ui->connectionLogText->append(remoteLog);
    ui->RestartServerButton->setEnabled(true);
    ui->forceDisconnectButton->setEnabled(false);
    ui->RestartServerButton->setText("Start Server");
    Server.close();
}

void MainWindow::restartServer(){
    ui->RestartServerButton->setEnabled(false);
    QString remoteLog = QDateTime::currentDateTime().toString("yyyy/MM/dd hh:mm:ss ");
    if(ui->RestartServerButton->text()=="Restart Server"){
        forceDisconnect();
        ui->RestartServerButton->setText("Restart Server");
        remoteLog.append(htmlGreen("========RemoteServerReStart!!========"));
    }else{
        ui->RestartServerButton->setText("Restart Server");
        remoteLog.append(htmlGreen("========RemoteServerstart!!========"));
    }
    Server.listen(QHostAddress::AnyIPv4,ServerPort);
    ui->forceDisconnectButton->setEnabled(true);
    ui->connectionLogText->append(remoteLog);
    remoteStatusLabel->setText(tr("Remote Server: Listening"));
    remoteStatusLedLabel->setPixmap(QPixmap("://images/led-orange.png"));
    refreshKey_slot();
    ui->RestartServerButton->setEnabled(true);
}

void MainWindow::cleanRemoteServerLog(){
    if(!ui->connectionLogText->document()->isEmpty()){
        ui->connectionLogText->document()->clear();
    }
}

void MainWindow::ExportRemoteServerLog(){
    QString remoteLog = QDateTime::currentDateTime().toString("yyyy/MM/dd hh:mm:ss ");
    QString dateNow = QDate::currentDate().toString("yyyy_MM_dd");
    QString saveLocation = QFileDialog::getSaveFileName(this,tr("saveFile"),"Remote-"+dateNow+".log",tr("log(*.log)"));
    if(saveLocation.isEmpty()) return;
    this->writeToFile(saveLocation,ui->connectionLogText->toPlainText());
    remoteLog.append(htmlGreen("Successful save at:"+saveLocation));
    ui->connectionLogText->append(remoteLog);
}

void MainWindow::timerTimeout(){
    disconnect(firstConnectTimer,SIGNAL(timeout()),this,SLOT(timerTimeout()));
    ServerConnection->write("remote|Verification fail|Timeout");
    ServerConnection->waitForBytesWritten();
    //serverDisconnected();
    //forceDisconnect();
    restartServer();
    QString remoteLog = QDateTime::currentDateTime().toString("yyyy/MM/dd hh:mm:ss ");
    remoteLog.append(htmlPurple("Reason : Verification fail(Timeout)"));
    ui->connectionLogText->append(remoteLog);
}

void MainWindow::prepareSend(){
    qDebug()<<"[PIT]prepareSend";
    connect(ServerConnection,SIGNAL(bytesWritten(qint64)),this,SLOT(updateClientProgress(qint64)));
    QString logsPath = getMinecraftLogsPath(m_mcServerPath);
    qDebug()<<"logsPath"<<logsPath;
    LF = new QFile(logsPath);
    if(!LF->open(QFile::ReadOnly)){
        QMessageBox::warning(this,QStringLiteral("應用程式"),
                             QStringLiteral("無法讀取 %1\n%2.").
                             arg(logsPath,LF->errorString()));
        return;
    }
    totalBytes = LF->size();//傳送檔案大小

    QDataStream sendOut(&outBlock,QIODevice::WriteOnly);
    sendOut.setVersion(QDataStream::Qt_4_6);//設定QT資料格式版本號
    QString CF = logsPath.right(logsPath.size()-//currentfile
                                logsPath.lastIndexOf("/")-1);
    sendOut<<qint64(0)<<qint64(0)<<CF;
    totalBytes +=outBlock.size();
    sendOut.device()->seek(0);//輸出指標移至最左端
    sendOut<<totalBytes<<qint64((outBlock.size()-sizeof(qint64)*2));//<<前面總長度,後面檔案名稱的長度
    bytesToWrite = totalBytes - ServerConnection->write(outBlock);//尚未傳送的bytes數
    qDebug()<<"totalBytes"<<totalBytes;
    outBlock.resize(0);//清空outBlock
}

void MainWindow::updateClientProgress(qint64 numBytes){
    qDebug()<<"[PIT]updateClientProgress";
    qDebug()<<"updateClientProgress"<<numBytes;
    bytesWritten += (int)numBytes;
    if(bytesToWrite > 0){
        //從檔案讀取Raw data, 儲存在outBlock中
        outBlock = LF->read(qMin(bytesToWrite,loadSize));
        //將outBlock的資料寫出去(資料網路)
        bytesToWrite -= (int)ServerConnection->write(outBlock);
        outBlock.resize(0);
    }else{
        disconnect(ServerConnection,SIGNAL(bytesWritten(qint64)),this,SLOT(updateClientProgress(qint64)));
        LF->close();
    }
}
void MainWindow::setClipboardContent(){
    QClipboard *board = QApplication::clipboard();
    ui->CopyButton->setEnabled(false);
    board->setText(ui->keyLineEdit->text());
    QMessageBox *msg = new QMessageBox(QMessageBox::Information,
                                       QStringLiteral("message"),
                                       QStringLiteral("Copy to clipboard complete!"));
    ui->CopyButton->setEnabled(true);
    msg->exec();
}
