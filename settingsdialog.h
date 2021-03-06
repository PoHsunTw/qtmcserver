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

#ifndef SETTINGSDIALOG_H
#define SETTINGSDIALOG_H

#include <QDialog>

namespace Ui {
class SettingsDialog;
}

class SettingsDialog : public QDialog
{
    Q_OBJECT
    
public:
    explicit SettingsDialog(QWidget *parent = 0);
    ~SettingsDialog();

    void initialize();

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

private slots:
    void on_downloadButton_clicked();
    void on_javaBrowseButton_clicked();
    void on_mcServerBrowseButton_clicked();
    void on_buttonBox_accepted();

    void accept();

private:
    Ui::SettingsDialog *ui;

    QString m_customJavaPath;
    QString m_mcServerPath;
    bool m_useCustomJavaPath;
    int m_xms;
    int m_xmx;
    QString m_additionalParameters;

};

#endif // SETTINGSDIALOG_H
