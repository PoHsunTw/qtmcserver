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

#ifndef DOWNLOADDIALOG_H
#define DOWNLOADDIALOG_H

#include <QDialog>

#include <QCoreApplication>
#include <QFile>
#include <QFileInfo>
#include <QList>
#include <QStringList>
#include <QTimer>
#include <QUrl>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>

#include <stdio.h>

class QSslError;

QT_USE_NAMESPACE

namespace Ui {
class DownloadDialog;
}

class DownloadDialog : public QDialog
{
    Q_OBJECT
    
public:
    explicit DownloadDialog(QWidget *parent = 0);
    ~DownloadDialog();

    void initialize();

    QString getSaveLocation() {return m_saveLocation;}

    void doDownload(const QUrl& url);
    QString saveFileName(const QUrl& url, const QString& path);
    bool saveToDisk(const QString &filename, QIODevice *data);

public slots:
    void startDownload();
    void downloadFinished(QNetworkReply *reply);

private slots:
    void on_downloadButton_clicked();
    void on_buttonBox_accepted();

private:
    Ui::DownloadDialog *ui;

    QString m_saveLocation;
    QString m_downloadPath;
    QNetworkAccessManager manager;
    QList<QNetworkReply *> currentDownloads;
};

#endif // DOWNLOADDIALOG_H
